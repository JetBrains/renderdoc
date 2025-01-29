import com.jetbrains.rd.framework.protocolOrThrow
import com.jetbrains.rd.util.lifetime.Lifetime
import com.jetbrains.rd.util.lifetime.waitTermination
import com.jetbrains.rd.util.reactive.fire
import com.jetbrains.rd.util.reactive.valueOrThrow
import com.jetbrains.rd.util.threading.coroutines.adviseSuspend
import com.jetbrains.rd.util.threading.coroutines.asCoroutineDispatcher
import com.jetbrains.renderdoc.rdClient.model.RdcLineBreakpoint
import com.jetbrains.renderdoc.rdClient.model.RdcActionFlags
import com.jetbrains.renderdoc.rdClient.model.RdcCapture
import kotlinx.coroutines.*
import org.junit.jupiter.api.Assertions.*
import kotlin.io.path.Path
import kotlin.io.path.name


class RenderDocClientMacosTest {
    companion object {
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
                debugSession.addLineBreakpoint.fire(RdcLineBreakpoint(0, 22u))
                debugSession.addLineBreakpoint.fire(RdcLineBreakpoint(0, 27u))
                debugSession.resume.fire()
                debugSession.resume.fire()
                debugSession.removeLineBreakpoint.fire(RdcLineBreakpoint(0, 27u))
                debugSession.resume.fire()
                debugSession.resume.fire()
                debugSession.resume.fire()
            }

            sessionLifetime.waitTermination()

            assertEquals(listOf(32u, 27u, 22u, 22u), lineNumbers)
        }

        suspend fun testRenderDocClient(lifetime: Lifetime, capture: RdcCapture) {
            val drawAction =
                capture.rootActions.first { it.flags.run { contains(RdcActionFlags.Drawcall) || contains(RdcActionFlags.MeshDispatch) } }
            assertDebugVertexStepByStep(lifetime, capture, drawAction.eventId)
            assertDebugVertexWithBreakpoint(lifetime, capture, drawAction.eventId)
        }
    }
}
