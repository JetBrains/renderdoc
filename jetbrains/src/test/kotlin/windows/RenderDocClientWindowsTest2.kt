package windows

import RenderDocAbstractClientTest
import com.jetbrains.rd.framework.protocolOrThrow
import com.jetbrains.rd.util.lifetime.Lifetime
import com.jetbrains.rd.util.lifetime.waitTermination
import com.jetbrains.rd.util.reactive.fire
import com.jetbrains.rd.util.threading.coroutines.adviseSuspend
import com.jetbrains.rd.util.threading.coroutines.asCoroutineDispatcher
import com.jetbrains.renderdoc.rdClient.model.RdcCapture
import com.jetbrains.renderdoc.rdClient.model.RdcDebugPixelInput
import com.jetbrains.renderdoc.rdClient.model.RdcDebugStack
import com.jetbrains.renderdoc.rdClient.model.RdcDebugVertexInput
import kotlinx.coroutines.withContext
import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.Test
import org.junit.jupiter.api.condition.EnabledOnOs
import org.junit.jupiter.api.condition.OS

@EnabledOnOs(OS.WINDOWS)
class RenderDocClientWindowsTest2 : RenderDocAbstractClientTest() {
    companion object {

        private fun assertActionsCollection(capture: RdcCapture) {
            Assertions.assertEquals(9, capture.rootActions.size)

            val fileUsages = getFileUsagesInEvent(capture)
            Assertions.assertEquals(1, fileUsages.size)

            assertSourceFilesUsages(
                fileUsages[1231u]!!,
                setOf("Assets/Shaders/050-059/050_Glass.shader"),
                setOf(
                    "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/HLSLSupport.cginc",
                    "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityShaderVariables.cginc",
                    "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityShaderUtilities.cginc",
                    "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityCG.cginc"
                )
            )
        }

        private suspend fun assertDebugVertexStepByStepDisassembly(modelLifetime: Lifetime, capture: RdcCapture) {
            // instantly finishing sessions
            assertSessionFinishesImmediately(modelLifetime, capture,
                RdcDebugVertexInput(0u, 0u, emptyList()), true)
            assertSessionFinishesImmediately(modelLifetime, capture,
                RdcDebugVertexInput(1231u, 36u, emptyList()), true)

            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            run {
                val sessionLifetime = modelLifetime.createNested()
                val debugSession = withContext(rdDispatcher) {
                    capture.debugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(1124u, 2u, emptyList()))
                }

                val drawCall = debugSession.sessionState.value?.drawCallSession
                Assertions.assertNotNull(drawCall)
                Assertions.assertTrue(drawCall!!.sourceFiles.isEmpty())

                val expectedLineNumbers = mutableListOf(22u)
                val lineNumbers = mutableListOf<UInt>()
                withContext(rdDispatcher) {
                    debugSession.sessionState.adviseSuspend(sessionLifetime, rdDispatcher) { state ->
                        if (state != null) {
                            lineNumbers.add(state.currentStack.lineStart)
                        } else {
                            sessionLifetime.terminate()
                        }
                    }
                    val steps = listOf(debugSession.stepOver, debugSession.stepInto, debugSession.stepOut)
                    for (i in 1..42) {
                        steps[i % 3].fire()
                        expectedLineNumbers.add(i.toUInt() + 22u)
                    }
                    debugSession.stepInto.fire()
                }

                sessionLifetime.waitTermination()

                Assertions.assertEquals(expectedLineNumbers, lineNumbers)
            }
        }

        private suspend fun assertDebugVertexStepByStepShaderLab(modelLifetime: Lifetime, capture: RdcCapture) {

            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.debugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(1231u, 35u, emptyList()))
            }

            val drawCall = debugSession.sessionState.value?.drawCallSession
            Assertions.assertNotNull(drawCall)
            Assertions.assertEquals("unnamed_shader", drawCall!!.sourceFiles[0].name)

            val frameTracker = RenderDocAbstractClientTest.Companion.FrameSessionTracker()
                .also { it.init(sessionLifetime, rdDispatcher, debugSession) }
            withContext(rdDispatcher) {
                debugSession.stepInto.fire()
                debugSession.stepOver.fire()
                debugSession.stepInto.fire()
                debugSession.stepInto.fire()
                debugSession.stepInto.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepInto.fire()
                debugSession.stepInto.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepInto.fire()
                debugSession.stepInto.fire()
                debugSession.stepInto.fire()
            }

            sessionLifetime.waitTermination()

            Assertions.assertEquals(
                listOf(
                    RdcDebugStack(1231u, 0, 0, 905u, 905u, 14u, 48u),
                    RdcDebugStack(1231u, 2, 0, 203u, 203u, 8u, 41u),
                    RdcDebugStack(1231u, 18, 0, 203u, 203u, 1u, 43u),
                    RdcDebugStack(1231u, 19, 0, 905u, 905u, 1u, 48u),
                    RdcDebugStack(1231u, 20, 0, 907u, 907u, 7u, 19u),
                    RdcDebugStack(1231u, 21, 0, 909u, 909u, 47u, 68u),
                    RdcDebugStack(1231u, 22, 0, 909u, 909u, 21u, 87u),
                    RdcDebugStack(1231u, 24, 0, 909u, 909u, 19u, 95u),
                    RdcDebugStack(1231u, 25, 0, 910u, 910u, 1u, 33u),
                    RdcDebugStack(1231u, 26, 0, 911u, 911u, 12u, 41u),
                    RdcDebugStack(1231u, 27, 0, 911u, 911u, 12u, 60u),
                    RdcDebugStack(1231u, 28, 0, 912u, 912u, 16u, 45u),
                    RdcDebugStack(1231u, 29, 0, 912u, 912u, 16u, 64u),
                    RdcDebugStack(1231u, 30, 0, 913u, 913u, 1u, 10u),
                ), frameTracker.frames
            )
            Assertions.assertEquals(hashMapOf(0 to 1231u), frameTracker.drawCallChanges)
            val expectedSourcesFull = listOf(listOf("unnamed_shader"))
            assertSourceNamesPerDrawCall(frameTracker, expectedSourcesFull, emptyList())
        }

        private suspend fun assertDebugVertexStepOutShaderLab(modelLifetime: Lifetime, capture: RdcCapture) {
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            run {
                val sessionLifetime = modelLifetime.createNested()
                val debugSession = withContext(rdDispatcher) {
                    capture.debugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(1231u, 30u, emptyList()))
                }

                val drawCall = debugSession.sessionState.value?.drawCallSession
                Assertions.assertNotNull(drawCall)
                Assertions.assertEquals("unnamed_shader", drawCall!!.sourceFiles[0].name)

                val frameTracker = RenderDocAbstractClientTest.Companion.FrameSessionTracker()
                    .also { it.init(sessionLifetime, rdDispatcher, debugSession) }
                withContext(rdDispatcher) {
                    debugSession.stepInto.fire()
                    debugSession.stepInto.fire()
                    debugSession.stepOut.fire()
                    debugSession.stepOut.fire()
                    debugSession.stepOver.fire()
                    debugSession.stepOut.fire()
                }

                sessionLifetime.waitTermination()

                Assertions.assertEquals(
                    listOf(
                        RdcDebugStack(1231u, 0, 0, 905u, 905u, 14u, 48u),
                        RdcDebugStack(1231u, 2, 0, 203u, 203u, 8u, 41u),
                        RdcDebugStack(1231u, 4, 0, 198u, 198u, 31u, 80u),
                        RdcDebugStack(1231u, 18, 0, 203u, 203u, 1u, 43u),
                        RdcDebugStack(1231u, 19, 0, 905u, 905u, 1u, 48u),
                        RdcDebugStack(1231u, 20, 0, 907u, 907u, 7u, 19u),
                    ), frameTracker.frames
                )
                Assertions.assertEquals(hashMapOf(0 to 1231u), frameTracker.drawCallChanges)
                val expectedSourcesFull = listOf(listOf("unnamed_shader"))
                assertSourceNamesPerDrawCall(frameTracker, expectedSourcesFull, emptyList())
            }

            run {
                val sessionLifetime = modelLifetime.createNested()
                val debugSession = withContext(rdDispatcher) {
                    capture.debugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(1231u, 30u, emptyList()))
                }

                val drawCall = debugSession.sessionState.value?.drawCallSession
                Assertions.assertNotNull(drawCall)
                Assertions.assertEquals("unnamed_shader", drawCall!!.sourceFiles[0].name)

                val frameTracker = RenderDocAbstractClientTest.Companion.FrameSessionTracker()
                    .also { it.init(sessionLifetime, rdDispatcher, debugSession) }
                withContext(rdDispatcher) {
                    debugSession.stepInto.fire()
                    debugSession.stepOut.fire()
                    debugSession.stepOver.fire()
                    debugSession.stepInto.fire()
                    debugSession.stepOver.fire()
                    debugSession.stepOut.fire()
                }

                sessionLifetime.waitTermination()

                Assertions.assertEquals(
                    listOf(
                        RdcDebugStack(1231u, 0, 0, 905u, 905u, 14u, 48u),
                        RdcDebugStack(1231u, 2, 0, 203u, 203u, 8u, 41u),
                        RdcDebugStack(1231u, 19, 0, 905u, 905u, 1u, 48u),
                        RdcDebugStack(1231u, 20, 0, 907u, 907u, 7u, 19u),
                        RdcDebugStack(1231u, 21, 0, 909u, 909u, 47u, 68u),
                        RdcDebugStack(1231u, 22, 0, 909u, 909u, 21u, 87u),
                    ), frameTracker.frames
                )
                Assertions.assertEquals(hashMapOf(0 to 1231u), frameTracker.drawCallChanges)
                val expectedSourcesFull = listOf(listOf("unnamed_shader"))
                assertSourceNamesPerDrawCall(frameTracker, expectedSourcesFull, emptyList())
            }
        }

        private suspend fun assertDebugPixelStepByStepShaderLab(modelLifetime: Lifetime, capture: RdcCapture) {
            // instantly finishing sessions
            assertSessionFinishesImmediately(modelLifetime, capture,
                RdcDebugPixelInput(0u, 0u, 0u, emptyList()), true)
            assertSessionFinishesImmediately(modelLifetime, capture,
                RdcDebugPixelInput(1159u, 297u, 336u, emptyList()), true)

            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.debugPixel.startSuspending(sessionLifetime, RdcDebugPixelInput(1231u, 392u, 28u, emptyList()))
            }
            val drawCallSession = debugSession.sessionState.value?.drawCallSession
            Assertions.assertNotNull(drawCallSession)
            Assertions.assertEquals("unnamed_shader", drawCallSession!!.sourceFiles[0].name)

            val frameTracker = RenderDocAbstractClientTest.Companion.FrameSessionTracker()
                .also { it.init(sessionLifetime, rdDispatcher, debugSession) }
            withContext(rdDispatcher) {
                debugSession.stepInto.fire()
                debugSession.stepInto.fire()
                debugSession.stepInto.fire()
                debugSession.stepInto.fire()
                debugSession.stepOver.fire()
                debugSession.stepOut.fire()
                debugSession.stepOut.fire()
                debugSession.stepOver.fire()
                debugSession.stepOut.fire()
            }

            sessionLifetime.waitTermination()

            Assertions.assertEquals(
                listOf(
                    RdcDebugStack(1231u, 0, 0, 918u, 918u, 14u, 61u),
                    RdcDebugStack(1231u, 1, 0, 918u, 918u, 29u, 59u),
                    RdcDebugStack(1231u, 2, 0, 918u, 918u, 14u, 61u),
                    RdcDebugStack(1231u, 3, 0, 706u, 706u, 8u, 45u),
                    RdcDebugStack(1231u, 5, 0, 696u, 696u, 1u, 36u),
                    RdcDebugStack(1231u, 6, 0, 699u, 699u, 15u, 35u),
                    RdcDebugStack(1231u, 12, 0, 706u, 706u, 1u, 47u),
                    RdcDebugStack(1231u, 13, 0, 918u, 918u, 7u, 66u),
                    RdcDebugStack(1231u, 14, 0, 919u, 919u, 17u, 27u),
                ), frameTracker.frames
            )
            Assertions.assertEquals(hashMapOf(0 to 1231u), frameTracker.drawCallChanges)

            val expectedSourcesFull = listOf(listOf("unnamed_shader"))
            assertSourceNamesPerDrawCall(frameTracker, expectedSourcesFull, emptyList())
        }
    }

    override val captureDirectoryPath: String = "samples/windows"
    override val driver: String = "D3D11"

    @Test
    fun test() {
        RenderDocClientWindowsTest2().run(54321L, "glass.rdc") { lifetime, file, capture ->
            assertActionsCollection(capture)
            assertDebugVertexStepByStepDisassembly(lifetime, capture)
            assertDebugVertexStepByStepShaderLab(lifetime, capture)
            assertDebugVertexStepOutShaderLab(lifetime, capture)
            assertDebugPixelStepByStepShaderLab(lifetime, capture)
        }
    }
}