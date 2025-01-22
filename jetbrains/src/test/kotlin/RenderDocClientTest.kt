import com.jetbrains.rd.framework.createBackgroundScheduler
import com.jetbrains.rd.framework.protocolOrThrow
import com.jetbrains.rd.util.lifetime.Lifetime
import com.jetbrains.rd.util.lifetime.waitTermination
import com.jetbrains.rd.util.reactive.fire
import com.jetbrains.rd.util.reactive.valueOrThrow
import com.jetbrains.rd.util.threading.coroutines.adviseSuspend
import com.jetbrains.rd.util.threading.coroutines.asCoroutineDispatcher
import com.jetbrains.rd.util.threading.coroutines.createTerminatedAfter
import com.jetbrains.renderdoc.rdClient.RenderDocClient
import com.jetbrains.renderdoc.rdClient.model.RdcLineBreakpoint
import com.jetbrains.renderdoc.rdClient.model.RdcActionFlags
import com.jetbrains.renderdoc.rdClient.model.RdcCapture
import com.jetbrains.renderdoc.rdClient.model.RdcSourceBreakpoint
import kotlinx.coroutines.*
import org.junit.jupiter.api.Assertions.*
import org.junit.jupiter.api.Test
import org.junit.jupiter.api.fail
import java.time.Duration
import kotlin.coroutines.EmptyCoroutineContext
import kotlin.io.path.Path
import kotlin.io.path.name
import kotlin.io.path.pathString
import kotlin.io.path.toPath


class RenderDocClientTest {
    @Test
    fun testRenderDocClient() {
        val sessionId = 12345L
        val lifetime = Lifetime.Eternal.createTerminatedAfter(Duration.ofSeconds(60), EmptyCoroutineContext)

        runBlocking {
            val scheduler = createBackgroundScheduler(lifetime, "RenderDocClient")
            val modelLifetime = lifetime.createNested()
            val client = RenderDocClient.createWithHost(modelLifetime, scheduler, sessionId, "build/libs/bin")
            val localReplaySupported = System.getenv("LOCAL_REPLAY_NOT_SUPPORTED") != "1"

            if (!localReplaySupported) {
                return@runBlocking
            }
            val model = client.model
            val rdDispatcher = scheduler.asCoroutineDispatcher

            val osName = System.getProperty("os.name").lowercase()

            fun getResourceInfo() = when {
                osName.contains("mac") -> Pair("macos", "Vulkan")
                osName.contains("win") -> Pair("windows", "D3D11")
                osName.contains("nix") || osName.contains("nux") || osName.contains("aix") -> Pair("linux", "D3D11")
                else -> fail("Tests can't be executed in current operating system")
            }

            val (os, driver) = getResourceInfo()

            val rdcSample = javaClass.classLoader.getResource("samples/${os}/test.rdc")?.toURI()?.toPath()?.pathString ?: fail("Failed to load sample resource")

            modelLifetime.usingNested { captureLifetime ->
                val captureFile = withContext(rdDispatcher) {
                    model.openCaptureFile.startSuspending(captureLifetime, rdcSample)
                }
                assertEquals(driver, driver)

                if (!captureFile.isLocalReplaySupported) {
                    fail("Local replay not supported. Set environment variable LOCAL_REPLAY_NOT_SUPPORTED=1 if this behavior is expected.")
                }

                val capture = withContext(rdDispatcher) {
                    captureFile.openCapture.startSuspending(captureLifetime, Unit)
                }
                when(os) {
                    "macos" -> RenderDocClientMacosTest.testRenderDocClient(modelLifetime, capture)
                    "windows" -> RenderDocClientWindowsTest.testRenderDocClient(modelLifetime, capture)
                }
            }
        }
    }
}
