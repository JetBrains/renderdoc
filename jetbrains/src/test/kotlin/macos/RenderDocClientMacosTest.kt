package macos

import RenderDocAbstractClientTest
import com.jetbrains.rd.framework.protocolOrThrow
import com.jetbrains.rd.util.lifetime.Lifetime
import com.jetbrains.rd.util.lifetime.waitTermination
import com.jetbrains.rd.util.reactive.fire
import com.jetbrains.rd.util.threading.coroutines.adviseSuspend
import com.jetbrains.rd.util.threading.coroutines.asCoroutineDispatcher
import com.jetbrains.renderdoc.rdClient.model.RdcActionFlags
import com.jetbrains.renderdoc.rdClient.model.RdcCapture
import com.jetbrains.renderdoc.rdClient.model.RdcDebugPixelInput
import com.jetbrains.renderdoc.rdClient.model.RdcDebugVertexInput
import com.jetbrains.renderdoc.rdClient.model.RdcLineBreakpoint
import kotlinx.coroutines.withContext
import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.Test
import org.junit.jupiter.api.condition.EnabledOnOs
import org.junit.jupiter.api.condition.OS
import kotlin.io.path.Path
import kotlin.io.path.name

@EnabledOnOs(OS.MAC)
class RenderDocClientMacosTest : RenderDocAbstractClientTest() {
    companion object {
        private fun assertActionsCollection(capture: RdcCapture) {
            Assertions.assertEquals(6, capture.rootActions.size)

            val fileUsages = getFileUsagesInEvent(capture)
            Assertions.assertEquals(0, fileUsages.size)

            Assertions.assertEquals(capture.rootActions[2].flags.single(), RdcActionFlags.Drawcall)
        }

        private suspend fun assertDebugVertexStepByStep(modelLifetime: Lifetime, capture: RdcCapture, eventId: UInt) {
            // instantly finishing sessions
            assertSessionFinishesImmediately(modelLifetime, capture,
                RdcDebugVertexInput(0u, 30u, emptyList()), true)

            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.debugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(eventId, 0u, emptyList()))
            }
            val drawCallSession = debugSession.sessionState.value!!.drawCallSession
            Assertions.assertNotNull(drawCallSession)
            Assertions.assertEquals("triangle.vert", Path(drawCallSession!!.sourceFiles[0].name).name)

            val lineNumbers = mutableListOf<UInt>()
            withContext(rdDispatcher) {
                debugSession.sessionState.adviseSuspend(sessionLifetime, rdDispatcher) { state ->
                    if (state != null) {
                        lineNumbers.add(state.currentStack.lineStart)
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

            Assertions.assertEquals(listOf(32u, 33u, 34u, 27u, 22u), lineNumbers)
        }

        private suspend fun assertDebugVertexWithBreakpoint(modelLifetime: Lifetime, capture: RdcCapture, eventId: UInt) {
            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.debugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(eventId, 0u, emptyList()))
            }

            val lineNumbers = mutableListOf<UInt>()
            withContext(rdDispatcher) {
                debugSession.sessionState.adviseSuspend(sessionLifetime, rdDispatcher) { state ->
                    if (state != null) {
                        lineNumbers.add(state.currentStack.lineStart)
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

            Assertions.assertEquals(listOf(32u, 27u, 22u, 22u), lineNumbers)
        }

        private suspend fun assertTryDebugVertex(modelLifetime: Lifetime, capture: RdcCapture) {
            assertSessionFinishesImmediately(modelLifetime, capture,
                RdcDebugVertexInput(0u, 30u, emptyList()), false)
            assertSessionFinishesImmediately(modelLifetime, capture,
                RdcDebugVertexInput(0u, 0u, emptyList()), false)
        }

        private suspend fun assertDebugPixelStepByStep(modelLifetime: Lifetime, capture: RdcCapture) {
            assertSessionFinishesImmediately(modelLifetime, capture,
                RdcDebugPixelInput(0u, 350u, 122u, emptyList()), true)
            assertSessionFinishesImmediately(modelLifetime, capture,
                RdcDebugPixelInput(0u, 2000u, 2000u, emptyList()), true)
        }

        private suspend fun assertTryDebugPixel(modelLifetime: Lifetime, capture: RdcCapture) {
            assertSessionFinishesImmediately(modelLifetime, capture,
                RdcDebugPixelInput(0u, 350u, 122u, emptyList()), false)
            assertSessionFinishesImmediately(modelLifetime, capture,
                RdcDebugPixelInput(0u, 2000u, 2000u, emptyList()), false)
        }

        private suspend fun assertVerticesTable(modelLifetime: Lifetime, capture: RdcCapture) {
            run {
                val vertices = withContext(capture.protocolOrThrow.scheduler.asCoroutineDispatcher) {
                    capture.getVertexStageInOutputs.startSuspending(modelLifetime, -1)
                }
                Assertions.assertNull(vertices)
            }
            run {
                val vertices = withContext(capture.protocolOrThrow.scheduler.asCoroutineDispatcher) {
                    capture.getVertexStageInOutputs.startSuspending(modelLifetime, 6)
                }
                Assertions.assertNull(vertices)
            }
            run {
                val vertices = withContext(capture.protocolOrThrow.scheduler.asCoroutineDispatcher) {
                    capture.getVertexStageInOutputs.startSuspending(modelLifetime, 14)
                }
                Assertions.assertNull(vertices)
            }
            run {
                val vertices = withContext(capture.protocolOrThrow.scheduler.asCoroutineDispatcher) {
                    capture.getVertexStageInOutputs.startSuspending(modelLifetime, 13)
                }
                Assertions.assertNotNull(vertices)
                Assertions.assertEquals(listOf(0u, 1u, 2u), vertices!!.input_indices)
                Assertions.assertEquals(listOf("inPos", "inColor"), vertices.input_columns)
                Assertions.assertEquals(listOf("gl_Position", "outColor"), vertices.output_columns)
                Assertions.assertEquals(listOf(listOf(1.0f, 1.0f, 0.0f), listOf(1.0f, 0.0f, 0.0f)), vertices.inputs[0])
                Assertions.assertEquals(listOf(listOf(-1.0f, 1.0f, 0.0f), listOf(0.0f, 1.0f, 0.0f)), vertices.inputs[1])
                Assertions.assertEquals(listOf(listOf(0.0f, -1.0f, 0.0f), listOf(0.0f, 0.0f, 1.0f)), vertices.inputs[2])
                Assertions.assertEquals(
                    listOf(
                        listOf(0.9742786f, 1.7320509f, 1.5058824f, 2.5f),
                        listOf(1.0f, 0.0f, 0.0f)
                    ), vertices.outputs[0]
                )
                Assertions.assertEquals(
                    listOf(
                        listOf(-0.9742786f, 1.7320509f, 1.5058824f, 2.5f),
                        listOf(0.0f, 1.0f, 0.0f)
                    ), vertices.outputs[1]
                )
                Assertions.assertEquals(
                    listOf(listOf(0.0f, -1.7320509f, 1.5058824f, 2.5f), listOf(0.0f, 0.0f, 1.0f)),
                    vertices.outputs[2]
                )
            }
        }

        private suspend fun assertTextureOutputs(modelLifetime: Lifetime, capture: RdcCapture) {
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val textureRoot = withContext(rdDispatcher) {
                capture.getPixelStageInOutputs.startSuspending(modelLifetime, -1)
            }
            withContext(rdDispatcher) {
                val textureLast = capture.getPixelStageInOutputs.startSuspending(modelLifetime, 16)!!
                Assertions.assertEquals(textureRoot, textureLast)

                Assertions.assertTrue(textureLast.inputs.isEmpty())
                Assertions.assertEquals(1, textureLast.colorOutputs.size)
                Assertions.assertNull(textureLast.depthOutput)
                Assertions.assertEquals(1280, textureLast.colorOutputs[0].width)
                Assertions.assertEquals(720, textureLast.colorOutputs[0].height)
            }
            withContext(rdDispatcher) {
                val textureLeaf = capture.getPixelStageInOutputs.startSuspending(modelLifetime, 13)!!
                Assertions.assertTrue(textureLeaf.inputs.isEmpty())
                Assertions.assertEquals(1, textureLeaf.colorOutputs.size)
                Assertions.assertNotNull(textureLeaf.depthOutput)
                Assertions.assertEquals(1280, textureLeaf.colorOutputs[0].width)
                Assertions.assertEquals(720, textureLeaf.colorOutputs[0].height)
            }
        }
    }

    override val captureDirectoryPath: String = "samples/macos"
    override val driver: String = "Vulkan"

    @Test
    fun test() = run(12345L, "test.rdc") { lifetime, file, capture ->
        assertActionsCollection(capture)
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