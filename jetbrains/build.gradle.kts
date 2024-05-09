import com.jetbrains.gradle.valueSources.VsVarsValueSource
import com.jetbrains.rd.generator.gradle.RdGenTask
import org.apache.tools.ant.filters.ReplaceTokens
import org.gradle.api.tasks.testing.logging.TestExceptionFormat
import org.gradle.api.tasks.testing.logging.TestLogEvent
import org.gradle.nativeplatform.platform.internal.ArchitectureInternal
import org.gradle.nativeplatform.platform.internal.Architectures
import org.gradle.nativeplatform.platform.internal.DefaultNativePlatform
import org.gradle.nativeplatform.platform.internal.OperatingSystemInternal
import org.jetbrains.kotlin.utils.addToStdlib.butIf
import org.jetbrains.kotlin.utils.addToStdlib.runIf
import kotlin.io.path.Path

val rdVersion by properties

repositories {
    if (rdVersion == "SNAPSHOT")
        mavenLocal()
    maven("https://cache-redirector.jetbrains.com/maven-central")
    maven { setUrl("https://packages.jetbrains.team/maven/p/ij/intellij-dependencies") }
}

plugins {
    id("com.jetbrains.rdgen")
    id("idea")
    id("maven-publish")

    kotlin("jvm") version "1.9.21"
}

apply {
    plugin("kotlin")
}

val rdGenRoot = File(projectDir, "src/generated")
val rdgen: SourceSet by sourceSets.creating {
    kotlin.setSrcDirs(listOf("src/rdgen/kotlin"))
}
val rdgenSourceSet = rdgen

sourceSets {
    main {
        kotlin {
            srcDir("src/generated/kotlin")
        }
    }
}

val dotnetRootNamespace = "JetBrains.RenderDoc.RdClient"
val dotnetProjectDir = File(projectDir, dotnetRootNamespace)

dependencies {
    val rdgenImplementation by configurations.getting
    rdgenImplementation("com.jetbrains.rd", "rd-gen", "$rdVersion")
    implementation("com.jetbrains.rd", "rd-framework", "$rdVersion")
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-core:1.8.0-RC2")

    testImplementation("org.junit.jupiter:junit-jupiter-api:5.10.0")
    testRuntimeOnly("org.junit.jupiter:junit-jupiter-engine:5.10.0")
}

val platformArchitecture = provider {
    val host = DefaultNativePlatform.host()
    val architecture = host.architecture
    // NativePlatform lies about ARM64 architecture on Windows ARM64 because Java (JDK 21) is x64 only
    return@provider if (host.operatingSystem.isWindows)
        Architectures.forInput(System.getenv("PROCESSOR_ARCHITECTURE"))
    else
        architecture
}

val hostOs = provider { DefaultNativePlatform.host().operatingSystem }

val targetArchitecture = platformArchitecture.map { arch ->
    System.getenv("RENDERDOC_TARGET_ARCHITECTURE")?.let { Architectures.forInput(it) }?.let { targetArch ->
        val os = hostOs.get()
        val isAvailable = arch == targetArch || os.isWindows && arch.isAmd64 && targetArch.isArm64
        if (!isAvailable) {
            throw GradleException("Cross-compilation only available for Windows ARM64 on Windows AMD64 (were $targetArch on $os $arch")
        }
        targetArch
    } ?: arch
}

fun ArchitectureInternal.asSlug() = when {
    isArm64 -> "aarch64"
    isAmd64 -> "x86_64"
    else -> throw UnsupportedOperationException("Unsupported architecture: $this")
}

fun OperatingSystemInternal.asSlug() = when {
    isWindows -> "windows"
    isLinux -> "linux"
    isMacOsX -> "macos"
    else -> throw UnsupportedOperationException("Unsupported operating system: $this")
}

val runtimeSuffix = provider {
    "${hostOs.get().asSlug()}-${targetArchitecture.get().asSlug()}"
}
val binaryName = hostOs.map { os -> "RenderDocHost".butIf(os.isWindows) { "$it.exe" } }
val nativeBinaryOutputDir = layout.buildDirectory.dir("libs/bin")

val generateModel by tasks.registering(RdGenTask::class) {
    val csharpGenDir = File(dotnetProjectDir, "Model")
    val kotlinGenDir = File(projectDir, "src/generated/kotlin/com/jetbrains/renderdoc/rdClient/model")
    val cppGenDir = File(projectDir, "rd-model")
    val rdgenRoot = "com.jetbrains.renderdoc.rd.gen.RenderDocRoot"

    rdgen {
        sources(rdgenSourceSet.kotlin.sourceDirectories)

        verbose = true
        packages = "com.jetbrains.renderdoc.rd.gen"
        hashFolder = "build/rdgen/renderdoc"

        generator {
            language = "cpp"
            transform = "asis"
            directory = "$cppGenDir"
            namespace = "jetbrains::renderdoc::model"
            root = rdgenRoot
        }

        generator {
            language = "kotlin"
            transform = "reversed"
            directory = "$kotlinGenDir"
            namespace = "com.jetbrains.renderdoc.rdClient.model"
            root = rdgenRoot
        }

        generator {
            language = "csharp"
            transform = "reversed"
            directory = "$csharpGenDir"
            namespace = "${dotnetRootNamespace}.Model"
            root = rdgenRoot
        }
    }

    copy {
        from("templates/Directory.Build.props", "templates/CMake.Variables.cmake")
        filter<ReplaceTokens>("tokens" to mapOf("RD_VERSION" to rdVersion))
        into(".")
    }
}

val buildRenderDocHostUnsigned by tasks.registering(Task::class) {
    dependsOn(generateModel)

    val cmakeConfig = if (project.findProperty("rdDebug") != "true") "Release" else "Debug"
    val cmakeBuildDir = "cmake-build-${cmakeConfig.lowercase()}"
    val binDirPath = Path("..", cmakeBuildDir, "bin")
    val binaryPath = binDirPath.resolve(binaryName.get())
    val isWindows = hostOs.get().isWindows

    doFirst {
        val platformArch = platformArchitecture.get()
        val targetArch = targetArchitecture.get()
        val isCrossCompile = platformArch != targetArch

        val generateCommandLine = mutableListOf(
            "cmake",
            ".",
            "-B",
            cmakeBuildDir,
            "-DJETBRAINS:BOOL=ON",
            "-DCMAKE_BUILD_TYPE=${cmakeConfig}",
            "-DENABLE_QRENDERDOC:BOOL=OFF",
            "-DENABLE_PYRENDERDOC:BOOL=OFF",
            "-DENABLE_PCH_HEADERS:BOOL=OFF",
            "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_${cmakeConfig.uppercase()}=${project.projectDir.toPath().resolve(binDirPath).normalize()}"
        )
        if (isCrossCompile) {
            generateCommandLine.add("-DCMAKE_TOOLCHAIN_FILE=${hostOs.get().asSlug()}-${platformArch.asSlug()}-${targetArch.asSlug()}-toolchain.cmake")
        } else {
            generateCommandLine.add("-G")
            generateCommandLine.add("Ninja")
        }

        project.findProperty("rdFetchPath")?.let { generateCommandLine.add("-DRD_FETCH_PATH=${it}") }

        val buildCommandLine = listOf("cmake", "--build", cmakeBuildDir, "--config", cmakeConfig)
        runIf(isWindows) {
            val vsVars = providers.of(VsVarsValueSource::class) {}.get()

            exec {
                workingDir(projectDir.parent)

                environment("CC" to "cl", "CXX" to "cl")

                fun ArchitectureInternal.asVsArch() = if (isArm64) "arm64" else "amd64"

                val vsArch = if (isCrossCompile) "${platformArch.asVsArch()}_${targetArch.asVsArch()}" else targetArch.asVsArch()
                val vsVarsCommand = "\"$vsVars\" $vsArch"
                val combinedCommand = "$vsVarsCommand && ${generateCommandLine.joinToString(" ")} && ${buildCommandLine.joinToString(" ")}"
                commandLine("cmd", "/c", combinedCommand)
            }
        } ?: run {
            exec {
                workingDir(projectDir.parent)

                commandLine(generateCommandLine)
            }

            exec {
                workingDir(projectDir.parent)

                commandLine(buildCommandLine)
            }
        }
    }

    outputs.upToDateWhen { false }
    outputs.file(binaryPath)
    runIf(isWindows) {
        outputs.file(binaryPath.parent.resolve("renderdoc.dll"))
    }
}

val buildRenderDocHost by tasks.registering(Task::class) {
    val outputDir = nativeBinaryOutputDir.get()
    inputs.files(buildRenderDocHostUnsigned)
    outputs.files(inputs.files.map { outputDir.file(it.name) })

    doLast {
        copy {
            from(buildRenderDocHostUnsigned)
            into(outputDir)
        }

        runIf(hostOs.get().isMacOsX) {
            outputs.files.forEach { unsignedFile ->
                exec {
                    commandLine("codesign", "-s", project.findProperty("signingIdentity") ?: "-", unsignedFile)
                }
            }
        }
    }
}

val runtimeJar by tasks.registering(Jar::class) {
    group = "build"

    from(buildRenderDocHost) {
        into("runtime/${runtimeSuffix.get()}")
    }

    archiveAppendix.set("runtime-${runtimeSuffix.get()}")
}

tasks {
    compileKotlin {
        dependsOn(generateModel)
    }

    test {
        useJUnitPlatform()

        inputs.files(buildRenderDocHost)

        testLogging {
            events = setOf(TestLogEvent.FAILED)
            exceptionFormat = TestExceptionFormat.FULL
        }
    }
}

publishing {
    publications {
        val baseName = "renderdoc-rd-client"
        val renderdocGroupId = "com.jetbrains.rd-client"

        create<MavenPublication>("client") {
            artifactId = baseName
            groupId = renderdocGroupId

            from(components["kotlin"])
            artifact(tasks["kotlinSourcesJar"])
        }

        create<MavenPublication>("runtime") {
            artifactId = "$baseName-runtime-${runtimeSuffix.get()}"
            groupId = renderdocGroupId

            artifact(runtimeJar)
        }

        create<MavenPublication>("runtimeAll") {
            artifactId = "$baseName-runtime-all"
            groupId = renderdocGroupId

            artifact(runtimeJar)
        }
    }
}
