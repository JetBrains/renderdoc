package com.jetbrains.gradle.valueSources

import org.apache.log4j.Logger
import org.gradle.api.IllegalDependencyNotation
import org.gradle.api.provider.Property
import org.gradle.api.provider.ValueSource
import org.gradle.api.provider.ValueSourceParameters
import org.gradle.process.ExecOperations
import org.gradle.process.internal.ExecException
import java.io.ByteArrayOutputStream
import java.nio.charset.Charset
import javax.inject.Inject
import kotlin.io.path.Path
import kotlin.io.path.exists

abstract class VsVarsValueSource : ValueSource<String, VsVarsValueSource.Parameters> {
    companion object {
        val PROGRAM_FILES_X86: String = System.getenv("ProgramFiles(x86)") ?: "C:\\Program Files (x86)"
        val PROGRAM_FILES: Array<String> = arrayOf(PROGRAM_FILES_X86, System.getenv("ProgramFiles") ?: "C:\\Program Files")
        val EDITIONS = arrayOf("Enterprise", "Professional", "Community", "BuildTools")
        val YEARS = arrayOf("2022", "2019", "2017")

        private val VsYearVersion = mapOf(
            "2022" to "17.0",
            "2019" to "16.0",
            "2017" to "15.0",
            "2015" to "14.0",
            "2013" to "12.0"
        )

        private fun vsVersionToVersionNumber(vsVersion: String): String =
            if (VsYearVersion.containsValue(vsVersion)) {
                vsVersion
            } else {
                VsYearVersion[vsVersion] ?: vsVersion
            }

        private fun vsVersionToYear(vsVersion: String): String {
            if (!VsYearVersion.containsKey(vsVersion)) {
                VsYearVersion.entries.firstOrNull { it.value == vsVersion }?.key?.let { return it }
            }
            return vsVersion
        }

        val logger = Logger.getLogger(VsVarsValueSource::class.java)
    }

    @get:Inject
    abstract val execOperations: ExecOperations

    fun findWithVsWhere(pattern: String, versionPattern: String): String? {
        val output = ByteArrayOutputStream()
        try {
            val execResult = execOperations.exec {
                isIgnoreExitValue = true
                commandLine("$PROGRAM_FILES_X86\\Microsoft Visual Studio\\Installer\\vswhere.exe", "-products", "*", versionPattern, "-prerelease", "-property", "installationPath")
                standardOutput = output
            }
            if (execResult.exitValue != 0)
                return null
            val vsInstallationPath = String(output.toByteArray(), Charset.defaultCharset()).trim()
            return "$vsInstallationPath\\$pattern"
        } catch (ex: ExecException) {
            logger.debug("vswhere failed with: ${ex.message}")
            return null
        }
    }

    override fun obtain(): String {
        val vsVersion = parameters.getVsVersion().getOrElse("latest")
        val vsVersionNumber = vsVersionToVersionNumber(vsVersion)
        val useSpecific = vsVersionNumber != "latest"
        val versionPattern = if (useSpecific) {
            val upperBound = vsVersionNumber.split('.')[0] + ".9"
            "-version \"${vsVersionNumber},${upperBound}"
        } else {
            "-latest"
        }

        // If vswhere is available, ask it about the location of the latest Visual Studio.
        val relativePathToVcVars = "VC\\Auxiliary\\Build\\vcvarsall.bat"
        var path = findWithVsWhere(relativePathToVcVars, versionPattern)
        if (path != null && Path(path).exists()) {
            logger.info("Found with vswhere: $path")
            return path
        }
        logger.info("Not found with vswhere")

        // If that does not work, try the standard installation locations,
        // starting with the latest and moving to the oldest.
        val years = if (!useSpecific) arrayOf(vsVersionToYear(vsVersion)) else YEARS
        for (programFiles in PROGRAM_FILES) {
            for (ver in years) {
                for (ed in EDITIONS) {
                    path = "${programFiles}\\Microsoft Visual Studio\\${ver}\\${ed}\\${relativePathToVcVars}"
                    logger.info("Trying standard location: $path")
                    if (Path(path).exists()) {
                        logger.info("Found standard location: $path")
                        return path
                    }
                }
            }
        }
        logger.info("Not found in standard locations")

        // Special case for Visual Studio 2015 (and maybe earlier), try it out too.
        path = "${PROGRAM_FILES_X86}\\Microsoft Visual C++ Build Tools\\vcbuildtools.bat"
        if (Path(path).exists()) {
            logger.info("Found VS 2015: $path")
            return path
        }
        logger.info("Not found in VS 2015 location: $path")

        throw IllegalDependencyNotation("Microsoft Visual Studio not found")
    }

    interface Parameters : ValueSourceParameters {
        fun getVsVersion(): Property<String>
    }
}