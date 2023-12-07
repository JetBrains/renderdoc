package com.jetbrains.renderdoc.rdClient

import com.jetbrains.rd.util.*
import com.jetbrains.rd.util.lifetime.Lifetime
import com.jetbrains.rd.util.lifetime.isAlive
import com.jetbrains.rd.util.threading.coroutines.launch
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.future.asDeferred
import java.io.UncheckedIOException
import java.nio.file.Paths
import java.util.concurrent.TimeUnit
import java.util.regex.Pattern
import kotlin.io.path.absolutePathString

@OptIn(ExperimentalCoroutinesApi::class)
internal class RenderDocHost(private val lifetime: Lifetime, binDir: String) {
    companion object {
        // spdlog message in format [date-time] [group] [logLevel] msg
        private val HostMessageRegex =
            Pattern.compile("^\\[[^]]+] \\[([^]]+)] \\[(trace|debug|info|warn|err|critical|off)] (.*)")
        private val HostIntroductionRegex = Pattern.compile("^HOST_INTRODUCTION: PORT=(\\d+)")
    }

    private lateinit var deferredExitCode: Deferred<Process>
    private lateinit var deferredPort: CompletableDeferred<Int>

    init {
        val binPath = Paths.get(binDir)

        val renderDocHostPath = binPath.resolve("RenderDocHost").absolutePathString()
        val processBuilder = ProcessBuilder()
            .command(renderDocHostPath)
            .redirectError(ProcessBuilder.Redirect.PIPE)
            .redirectOutput(ProcessBuilder.Redirect.PIPE)
            .redirectInput(ProcessBuilder.Redirect.PIPE)

        lifetime.executeOrThrow {
            val serverLogger = getLogger("RenderDocHost")
            val process = processBuilder.start()
            deferredExitCode = process.onExit().asDeferred()
            deferredPort = CompletableDeferred(deferredExitCode)

            deferredExitCode.invokeOnCompletion {
                serverLogger.info { "RenderDocHost exited with ${process.exitValue()} code" }
            }

            // Detailed methods to handle processDataReceived, errorDataReceived and logHostMessage are omitted for brevity
            var introduced = false
            val watchesDispatchers = Dispatchers.IO.limitedParallelism(2)
            lifetime.launch(watchesDispatchers) {
                try {
                    process.inputReader().lines().forEach { line ->
                        if (!introduced) {
                            val matcher = HostIntroductionRegex.matcher(line)
                            if (matcher.find()) {
                                deferredPort.complete(matcher.group(1).toInt())
                                introduced = true
                                return@forEach
                            }
                        }

                        serverLogger.debug {
                            logHostMessage(serverLogger, LogLevel.Debug, line)
                        }
                    }
                } catch (ex: UncheckedIOException) {
                    if (lifetime.isAlive)
                        throw ex
                }
            }

            lifetime.launch(watchesDispatchers) {
                try {
                    process.errorReader().lines().forEach {
                        logHostMessage(serverLogger, LogLevel.Error, it)
                    }
                } catch (ex: UncheckedIOException) {
                    if (lifetime.isAlive)
                        throw ex
                }
            }

            lifetime.onTermination {
                process.destroy()
                process.waitFor(1, TimeUnit.SECONDS)
            }
        }
    }

    suspend fun getPort() = deferredPort.await()

    private fun logHostMessage(serverLogger: Logger, defaultLogLevel: LogLevel, message: String) {
        val matcher = HostMessageRegex.matcher(message)
        if (matcher.find()) {
            val logLevel = when (matcher.group(2)) {
                "trace" -> LogLevel.Trace
                "debug" -> LogLevel.Debug
                "info" -> LogLevel.Info
                "warn" -> LogLevel.Warn
                "err" -> LogLevel.Error
                "critical" -> LogLevel.Fatal
                else -> defaultLogLevel
            }
            serverLogger.log(logLevel, "${matcher.group(1)} | ${matcher.group(3)}", null)
        } else {
            serverLogger.log(defaultLogLevel, message, null)
        }
    }
}