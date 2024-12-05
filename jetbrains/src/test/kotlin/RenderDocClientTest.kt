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
    private suspend fun assertDebugVertexStepByStep(modelLifetime: Lifetime, capture: RdcCapture, eventId: UInt) {
        val sessionLifetime = modelLifetime.createNested()
        val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
        val debugSession = withContext(rdDispatcher) {
            capture.debugVertex.startSuspending(sessionLifetime, eventId)
        }
        assertEquals("triangle.vert", Path(debugSession.drawCallSession.valueOrThrow.sourceFiles[0].name).name)

        val lineNumbers = mutableListOf<UInt>()
        withContext(rdDispatcher) {
            debugSession.currentStack.adviseSuspend(sessionLifetime, rdDispatcher) {
                if (it != null) {
                    lineNumbers.add(it.lineStart)
                } else {
                    sessionLifetime.terminate()
                }
            }
            debugSession.stepInto.fire()
            debugSession.stepOver.fire()
            debugSession.stepInto.fire()
            debugSession.stepInto.fire()
            debugSession.stepInto.fire()
        }

        sessionLifetime.waitTermination()

        assertEquals(listOf(32u, 33u, 34u, 27u, 22u), lineNumbers)
    }

    private suspend fun assertDebugVertexWithBreakpoint(modelLifetime: Lifetime, capture: RdcCapture, eventId: UInt) {
        val sessionLifetime = modelLifetime.createNested()
        val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
        val debugSession = withContext(rdDispatcher) {
            capture.debugVertex.startSuspending(sessionLifetime, eventId)
        }

        val lineNumbers = mutableListOf<UInt>()
        withContext(rdDispatcher) {
            debugSession.currentStack.adviseSuspend(sessionLifetime, rdDispatcher) {
                if (it != null) {
                    lineNumbers.add(it.lineStart)
                } else {
                    sessionLifetime.terminate()
                }
            }
            debugSession.addLineBreakpoint.fire(RdcLineBreakpoint(0u, 22u))
            debugSession.addLineBreakpoint.fire(RdcLineBreakpoint(0u, 27u))
            debugSession.resume.fire()
            debugSession.resume.fire()
            debugSession.removeLineBreakpoint.fire(RdcLineBreakpoint(0u, 27u))
            debugSession.resume.fire()
            debugSession.resume.fire()
            debugSession.resume.fire()
        }

        sessionLifetime.waitTermination()

        assertEquals(listOf(32u, 27u, 22u, 22u), lineNumbers)
    }

    @Test
    fun testRenderDocClient() {
        val sessionId = 12345L
        val lifetime = Lifetime.Eternal.createTerminatedAfter(Duration.ofSeconds(60), EmptyCoroutineContext)

        runBlocking {
            val scheduler = createBackgroundScheduler(lifetime, "RenderDocClient")
            val modelLifetime = lifetime.createNested()
            val client = RenderDocClient.createWithHost(modelLifetime, scheduler, sessionId, "build/libs/bin")
            val localReplaySupported = System.getenv("LOCAL_REPLAY_NOT_SUPPORTED") != "1"
            val model = client.model
            val rdDispatcher = scheduler.asCoroutineDispatcher
            val rdcSample = javaClass.classLoader.getResource("samples/macos/test.rdc")?.toURI()?.toPath()?.pathString ?: fail("Failed to load sample resource")

            modelLifetime.usingNested { captureLifetime ->
                val captureFile = withContext(rdDispatcher) {
                    model.openCaptureFile.startSuspending(captureLifetime, rdcSample)
                }
                assertEquals(captureFile.driverName, "Vulkan")

                if (!localReplaySupported) {
                    return@runBlocking
                }

                if (!captureFile.isLocalReplaySupported) {
                    fail("Local replay not supported. Set environment variable LOCAL_REPLAY_NOT_SUPPORTED=1 if this behavior is expected.")
                }

                val capture = withContext(rdDispatcher) {
                    captureFile.openCapture.startSuspending(captureLifetime, Unit)
                }

                val drawAction = capture.rootActions.first { it.flags.run { contains(RdcActionFlags.Drawcall) || contains(RdcActionFlags.MeshDispatch) }}
                assertDebugVertexStepByStep(modelLifetime, capture, drawAction.eventId)
                assertDebugVertexWithBreakpoint(modelLifetime, capture, drawAction.eventId)
            }
        }
    }
}
