import com.jetbrains.rd.framework.protocolOrThrow
import com.jetbrains.rd.util.lifetime.Lifetime
import com.jetbrains.rd.util.lifetime.waitTermination
import com.jetbrains.rd.util.reactive.fire
import com.jetbrains.rd.util.threading.coroutines.adviseSuspend
import com.jetbrains.rd.util.threading.coroutines.asCoroutineDispatcher
import com.jetbrains.renderdoc.rdClient.model.*
import kotlinx.coroutines.*
import org.junit.jupiter.api.Assertions.*
import kotlin.io.path.Path
import kotlin.io.path.name


class RenderDocClientMacosTest {
    companion object {
        private suspend fun assertDebugVertexStepByStep(modelLifetime: Lifetime, capture: RdcCapture, eventId: UInt) {
            // instantly finishing sessions
            RenderDocClientTest.assertSessionFinishesImmediately(modelLifetime, capture, RdcDebugVertexInput(0u, 30u, emptyList()), true)

            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.debugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(eventId, 0u, emptyList()))
            }
            assertEquals("triangle.vert", Path(debugSession.drawCallSession.value!!.sourceFiles[0].name).name)

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
                capture.debugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(eventId, 0u, emptyList()))
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

        private suspend fun assertTryDebugVertex(modelLifetime: Lifetime, capture: RdcCapture) {
            RenderDocClientTest.assertSessionFinishesImmediately(modelLifetime, capture, RdcDebugVertexInput(0u, 30u, emptyList()), false)
            RenderDocClientTest.assertSessionFinishesImmediately(modelLifetime, capture, RdcDebugVertexInput(0u, 0u, emptyList()), false)
        }

        private suspend fun assertDebugPixelStepByStep(modelLifetime: Lifetime, capture: RdcCapture) {
            RenderDocClientTest.assertSessionFinishesImmediately(modelLifetime, capture, RdcDebugPixelInput(0u, 350u, 122u, emptyList()), true)
            RenderDocClientTest.assertSessionFinishesImmediately(modelLifetime, capture, RdcDebugPixelInput(0u, 2000u, 2000u, emptyList()), true)
        }

        private suspend fun assertTryDebugPixel(modelLifetime: Lifetime, capture: RdcCapture) {
            RenderDocClientTest.assertSessionFinishesImmediately(modelLifetime, capture, RdcDebugPixelInput(0u, 350u, 122u, emptyList()), false)
            RenderDocClientTest.assertSessionFinishesImmediately(modelLifetime, capture, RdcDebugPixelInput(0u, 2000u, 2000u, emptyList()), false)
        }

        private suspend fun assertVerticesTable(modelLifetime: Lifetime, capture: RdcCapture) {
            run {
                val vertices = withContext(capture.protocolOrThrow.scheduler.asCoroutineDispatcher) {
                    capture.getVertexStageInOutputs.startSuspending(modelLifetime, -1)
                }
                assertNull(vertices)
            }
            run {
                val vertices = withContext(capture.protocolOrThrow.scheduler.asCoroutineDispatcher) {
                    capture.getVertexStageInOutputs.startSuspending(modelLifetime, 6)
                }
                assertNull(vertices)
            }
            run {
                val vertices = withContext(capture.protocolOrThrow.scheduler.asCoroutineDispatcher) {
                    capture.getVertexStageInOutputs.startSuspending(modelLifetime, 14)
                }
                assertNull(vertices)
            }
            run {
                val vertices = withContext(capture.protocolOrThrow.scheduler.asCoroutineDispatcher) {
                    capture.getVertexStageInOutputs.startSuspending(modelLifetime, 13)
                }
                assertNotNull(vertices)
                assertEquals(listOf(0u, 1u, 2u), vertices!!.input_indices)
                assertEquals(listOf("inPos", "inColor"), vertices.input_columns)
                assertEquals(listOf("gl_Position", "outColor"), vertices.output_columns)
                assertEquals(listOf(listOf(1.0f, 1.0f, 0.0f), listOf(1.0f, 0.0f, 0.0f)), vertices.inputs[0])
                assertEquals(listOf(listOf(-1.0f, 1.0f, 0.0f), listOf(0.0f, 1.0f, 0.0f)), vertices.inputs[1])
                assertEquals(listOf(listOf(0.0f, -1.0f, 0.0f), listOf(0.0f, 0.0f, 1.0f)), vertices.inputs[2])
                assertEquals(listOf(listOf(0.9742786f, 1.7320509f, 1.5058824f, 2.5f), listOf(1.0f, 0.0f, 0.0f)), vertices.outputs[0])
                assertEquals(listOf(listOf(-0.9742786f, 1.7320509f, 1.5058824f, 2.5f), listOf(0.0f, 1.0f, 0.0f)), vertices.outputs[1])
                assertEquals(listOf(listOf(0.0f, -1.7320509f, 1.5058824f, 2.5f), listOf(0.0f, 0.0f, 1.0f)), vertices.outputs[2])
            }
        }

        private suspend fun assertTextureOutputs(modelLifetime: Lifetime, capture: RdcCapture) {
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val textureRoot = withContext(rdDispatcher) {
                capture.getTextureRGBBuffer.startSuspending(modelLifetime, -1)
            }
            withContext(rdDispatcher) {
                val textureLast = capture.getTextureRGBBuffer.startSuspending(modelLifetime, 16)
                assertEquals(textureRoot, textureLast)
                assertEquals(1, textureLast!!.colorOutputs.size)
                assertNull(textureLast.depthOutput)
                assertEquals(1280, textureLast.colorOutputs[0].width)
                assertEquals(720, textureLast.colorOutputs[0].height)
            }
            withContext(rdDispatcher) {
                val textureLeaf = capture.getTextureRGBBuffer.startSuspending(modelLifetime, 13)
                assertEquals(1, textureLeaf!!.colorOutputs.size)
                assertNotNull(textureLeaf.depthOutput)
                assertEquals(1280, textureLeaf.colorOutputs[0].width)
                assertEquals(720, textureLeaf.colorOutputs[0].height)
            }
        }

        suspend fun testRenderDocClient(lifetime: Lifetime, capture: RdcCapture) {
            val drawAction =
                capture.rootActions.first { it.flags.run { contains(RdcActionFlags.Drawcall) || contains(RdcActionFlags.MeshDispatch) } }
            assertDebugVertexStepByStep(lifetime, capture, drawAction.eventId)
            assertDebugVertexWithBreakpoint(lifetime, capture, drawAction.eventId)
            assertTryDebugVertex(lifetime, capture)

            assertDebugPixelStepByStep(lifetime, capture)
            assertTryDebugPixel(lifetime, capture)

            assertVerticesTable(lifetime, capture)
            assertTextureOutputs(lifetime, capture)
        }
    }
}
