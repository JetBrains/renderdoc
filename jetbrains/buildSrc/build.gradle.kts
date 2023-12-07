repositories {
    mavenCentral()
    maven { url = uri("https://packages.jetbrains.team/maven/p/jcs/maven") }
}

plugins {
    `kotlin-dsl`
}

dependencies {
    implementation(gradleApi())
}