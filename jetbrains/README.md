# Installation & Usage instructions

This is a hybrid project which has bundled C++ RD host and .NET/Kotlin RD client. 

## Pre-Requisites

* [CMake](https://cmake.org/)

The build also uses [Gradle](https://gradle.org/), but it should be automatically installed with `gradlew` script.

## Important Notes

Normally you should just run a build and automatically build all dependencies, but with incremental Rider/ReSharper build you 
may have issue with skipping project's build and it's BeforeTarget dependencies (like BuildRenderDocHost for JetBrains.RenderDoc.RdClient.csproj).

You may either manually rebuild RenderDocHost in case of changes in C++ or RD model with `./gradlew buildRenderDocHost` or `./gradlew generateModel`
or disable ReSharper Build in settings.