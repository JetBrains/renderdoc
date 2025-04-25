import com.jetbrains.rd.framework.protocolOrThrow
import com.jetbrains.rd.util.lifetime.Lifetime
import com.jetbrains.rd.util.lifetime.waitTermination
import com.jetbrains.rd.util.reactive.fire
import com.jetbrains.rd.util.threading.coroutines.adviseSuspend
import com.jetbrains.rd.util.threading.coroutines.asCoroutineDispatcher
import com.jetbrains.renderdoc.rdClient.model.*
import kotlinx.coroutines.*
import org.junit.jupiter.api.Assertions.*
import RenderDocClientTest.Companion.FrameSessionTracker
import RenderDocClientTest.Companion.assertSourceFilesUsages
import RenderDocClientTest.Companion.assertSourceNamesPerDrawCall
import RenderDocClientTest.Companion.getFileUsagesInEvent


class RenderDocClientWindowsTest {
    companion object {

        private fun assertActionsCollection(capture: RdcCapture) {
            assertEquals(13, capture.rootActions.size)

            val fileUsages = getFileUsagesInEvent(capture)
            assertEquals(4, fileUsages.size)

            assertSourceFilesUsages(
                fileUsages[715u]!!,
                setOf("Assets/ShaderForSphere.shader"),
                setOf(
                "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/HLSLSupport.cginc",
                "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityShaderVariables.cginc",
                "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityShaderUtilities.cginc",
                "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityCG.cginc"))

            assertSourceFilesUsages(
                fileUsages[732u]!!,
                setOf("Assets/NewShader.shader"),
                setOf(
                    "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/HLSLSupport.cginc",
                    "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityShaderVariables.cginc",
                    "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityShaderUtilities.cginc",
                    "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityCG.cginc"))

            assertSourceFilesUsages(
                fileUsages[749u]!!,
                setOf("Assets/Cube Shader.shader"),
                setOf(
                    "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/HLSLSupport.cginc",
                    "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityShaderVariables.cginc",
                    "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityShaderUtilities.cginc",
                    "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityCG.cginc",
                    "Assets/mult.hlsl"))

            assertSourceFilesUsages(
                fileUsages[765u]!!,
                setOf("Assets/Waves.shader"),
                setOf(
                    "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/HLSLSupport.cginc",
                    "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityShaderVariables.cginc",
                    "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityShaderUtilities.cginc",
                    "C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityCG.cginc"))
        }

        private suspend fun assertDebugVertexStepByStepDisassembly(modelLifetime: Lifetime, capture: RdcCapture) {
            // instantly finishing sessions
            RenderDocClientTest.assertSessionFinishesImmediately(modelLifetime, capture, RdcDebugVertexInput(0u, 0u, emptyList()), true)

            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            run {
                val sessionLifetime = modelLifetime.createNested()
                val debugSession = withContext(rdDispatcher) {
                    capture.debugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(784u, 0u, emptyList()))
                }

                val drawCall = debugSession.sessionState.value?.drawCallSession
                assertNotNull(drawCall)
                assertTrue(drawCall!!.sourceFiles.isEmpty())

                val expectedLineNumbers = mutableListOf(15u)
                val lineNumbers = mutableListOf<UInt>()
                withContext(rdDispatcher) {
                    debugSession.sessionState.adviseSuspend(sessionLifetime, rdDispatcher) { state ->
                        if (state != null) {
                            lineNumbers.add(state.currentStack.lineStart)
                        } else {
                            sessionLifetime.terminate()
                        }
                    }
                    repeat(106) {
                        debugSession.stepOver.fire()
                        expectedLineNumbers.add(it.toUInt() + 16u)
                    }
                    repeat(17) {
                        debugSession.stepInto.fire()
                        expectedLineNumbers.add(it.toUInt() + 163u)
                    }
                    debugSession.stepInto.fire()
                }

                sessionLifetime.waitTermination()

                assertEquals(expectedLineNumbers, lineNumbers)
            }

            run {
                val sessionLifetime = modelLifetime.createNested()
                val debugSession = withContext(rdDispatcher) {
                    capture.debugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(784u, 5039u, emptyList()))
                }

                val drawCall = debugSession.sessionState.value?.drawCallSession
                assertNotNull(drawCall)
                assertTrue(drawCall!!.sourceFiles.isEmpty())

                val expectedLineNumbers = mutableListOf(15u)
                val lineNumbers = mutableListOf<UInt>()
                withContext(rdDispatcher) {
                    debugSession.sessionState.adviseSuspend(sessionLifetime, rdDispatcher) { state ->
                        if (state != null) {
                            lineNumbers.add(state.currentStack.lineStart)
                        } else {
                            sessionLifetime.terminate()
                        }
                    }
                    repeat(21) {
                        debugSession.stepOver.fire()
                        expectedLineNumbers.add(it.toUInt() + 16u)
                    }
                    repeat(58) {
                        debugSession.stepInto.fire()
                        expectedLineNumbers.add(it.toUInt() + 122u)
                    }
                    debugSession.stepInto.fire()
                }

                sessionLifetime.waitTermination()

                assertEquals(expectedLineNumbers, lineNumbers)
            }
        }

        private suspend fun assertDebugVertexStepByStepShaderLab(modelLifetime: Lifetime, capture: RdcCapture) {
            // instantly finishing sessions
            RenderDocClientTest.assertSessionFinishesImmediately(modelLifetime, capture, RdcDebugVertexInput(66u, 10000u, emptyList()), true)

            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.debugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(732u, 30u, emptyList()))
            }

            val drawCall = debugSession.sessionState.value?.drawCallSession
            assertNotNull(drawCall)
            assertEquals("unnamed_shader", drawCall!!.sourceFiles[0].name)

            val frameTracker = FrameSessionTracker().also { it.init(sessionLifetime, rdDispatcher, debugSession) }
            withContext(rdDispatcher) {
                debugSession.stepInto.fire()
                debugSession.stepInto.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepInto.fire()
                debugSession.stepInto.fire()
                debugSession.stepInto.fire()
                debugSession.stepInto.fire()
            }

            sessionLifetime.waitTermination()

            assertEquals(listOf(
                RdcDebugStack(732u, 0, 0, 918u, 918u, 11u, 45u),
                RdcDebugStack(732u, 2, 0, 226u, 226u, 8u, 41u),
                RdcDebugStack(732u, 4, 0, 221u, 221u, 31u, 80u),
                RdcDebugStack(732u, 11, 0, 221u, 221u, 8u, 82u),
                RdcDebugStack(732u, 18, 0, 226u, 226u, 1u, 43u),
                RdcDebugStack(732u, 19, 0, 918u, 918u, 1u, 45u),
                RdcDebugStack(732u, 20, 0, 919u, 919u, 13u, 35u),
                RdcDebugStack(732u, 21, 0, 920u, 920u, 1u, 10u)
                ), frameTracker.frames
            )
            assertEquals(hashMapOf(0 to 732u), frameTracker.drawCallChanges)
            val expectedSourcesFull = listOf(listOf("unnamed_shader"))
            assertSourceNamesPerDrawCall(frameTracker, expectedSourcesFull, emptyList())
        }

        private suspend fun assertDebugVertexWithBreakpoints(modelLifetime: Lifetime, capture: RdcCapture) {
            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.debugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(749u, 0u, emptyList()))
            }

            val frameTracker = FrameSessionTracker().also { it.init(sessionLifetime, rdDispatcher, debugSession) }
            withContext(rdDispatcher) {
                debugSession.addSourceBreakpoint.fire(RdcSourceBreakpoint("Assets/Cube Shader.shader", 57u))
                debugSession.addSourceBreakpoint.fire(RdcSourceBreakpoint("Assets/Cube Shader.shader", 62u))
                debugSession.resume.fire()
                debugSession.resume.fire()

                debugSession.addSourceBreakpoint.fire(RdcSourceBreakpoint("Assets/mult.hlsl", 7u))
                debugSession.stepOver.fire()

                debugSession.addSourceBreakpoint.fire(RdcSourceBreakpoint("Assets/Cube Shader.shader", 64u))
                debugSession.resume.fire()
                debugSession.resume.fire()
                debugSession.resume.fire()
            }

            sessionLifetime.waitTermination()
            assertEquals(listOf(
                RdcDebugStack(749u, 0, 0, 926u, 926u, 14u, 48u),
                RdcDebugStack(749u, 20, 0, 930u, 934u, 5u, 19u),
                RdcDebugStack(749u, 21, 0, 934u, 934u, 3u, 19u),
                RdcDebugStack(749u, 24, 0, 904u, 904u, 8u, 12u),
                RdcDebugStack(749u, 26, 0, 930u, 934u, 5u, 19u),
                RdcDebugStack(749u, 32, 0, 936u, 936u, 1u, 10u),
            ), frameTracker.frames
            )
            assertEquals(hashMapOf(0 to 749u), frameTracker.drawCallChanges)
            val expectedSourcesFull = listOf(listOf("unnamed_shader"))
            assertSourceNamesPerDrawCall(frameTracker, expectedSourcesFull, emptyList())
        }

        private suspend fun assertTryDebugVertexStepByStep(modelLifetime: Lifetime, capture: RdcCapture, breakpoints: List<RdcSourceBreakpoint>) {
            // instantly finishing sessions
            RenderDocClientTest.assertSessionFinishesImmediately(modelLifetime, capture, RdcDebugVertexInput(0u, 15000u, breakpoints), false)

            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.tryDebugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(0u, 35u, breakpoints))
            }

            val frameTracker = FrameSessionTracker().also { it.init(sessionLifetime, rdDispatcher, debugSession) }
            withContext(rdDispatcher) {
                // event 715
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()

                debugSession.stepInto.fire()

                //event 732
                debugSession.stepInto.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()

                debugSession.stepOver.fire()
                // event 749
                repeat(11) {
                    debugSession.stepOver.fire()
                }
                debugSession.stepOver.fire()

                //event 765
                debugSession.stepInto.fire()
                debugSession.stepInto.fire()
                repeat(10) {
                    debugSession.stepOver.fire()
                }
                debugSession.stepInto.fire()

                //event 784
                debugSession.stepInto.fire()
                debugSession.stepInto.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.resume.fire()
            }

            sessionLifetime.waitTermination()

            assertEquals(listOf(
                RdcDebugStack(715u, 0, 0, 883u, 883u, 11u, 45u),
                RdcDebugStack(715u, 19, 0, 883u, 883u, 1u, 45u),
                RdcDebugStack(715u, 20, 0, 884u, 884u, 13u, 28u),
                RdcDebugStack(715u, 21, 0, 884u, 884u, 13u, 34u),
                RdcDebugStack(715u, 22, 0, 885u, 885u, 1u, 10u),
                RdcDebugStack(715u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(732u, -1, -1, 0u, 0u, 0u, 0u),
                RdcDebugStack(732u, 0, 0, 918u, 918u, 11u, 45u),
                RdcDebugStack(732u, 19, 0, 918u, 918u, 1u, 45u),
                RdcDebugStack(732u, 20, 0, 919u, 919u, 13u, 35u),
                RdcDebugStack(732u, 21, 0, 920u, 920u, 1u, 10u),
                RdcDebugStack(732u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(749u, -1, -1, 0u, 0u, 0u, 0u),
                RdcDebugStack(749u, 0, 0, 926u, 926u, 14u, 48u),
                RdcDebugStack(749u, 19, 0, 926u, 926u, 1u, 48u),
                RdcDebugStack(749u, 20, 0, 930u, 934u, 5u, 19u),
                RdcDebugStack(749u, 21, 0, 934u, 934u, 3u, 19u),
                RdcDebugStack(749u, 24, 0, 904u, 904u, 8u, 12u),
                RdcDebugStack(749u, 26, 0, 930u, 934u, 5u, 19u),
                RdcDebugStack(749u, 28, 0, 935u, 935u, 12u, 51u),
                RdcDebugStack(749u, 29, 0, 935u, 935u, 12u, 70u),
                RdcDebugStack(749u, 30, 0, 935u, 935u, 76u, 91u),
                RdcDebugStack(749u, 31, 0, 935u, 935u, 10u, 91u),
                RdcDebugStack(749u, 32, 0, 936u, 936u, 1u, 10u),
                RdcDebugStack(749u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(765u, -1, -1, 0u, 0u, 0u, 0u),
                RdcDebugStack(765u, 0, 0, 895u, 895u, 19u, 58u),
                RdcDebugStack(765u, 7, 0, 897u, 897u, 20u, 48u),
                RdcDebugStack(765u, 8, 0, 897u, 897u, 52u, 73u),
                RdcDebugStack(765u, 9, 0, 897u, 897u, 20u, 73u),
                RdcDebugStack(765u, 10, 0, 897u, 897u, 14u, 75u),
                RdcDebugStack(765u, 11, 0, 897u, 897u, 14u, 92u),
                RdcDebugStack(765u, 12, 0, 899u, 899u, 1u, 22u),
                RdcDebugStack(765u, 13, 0, 901u, 901u, 11u, 45u),
                RdcDebugStack(765u, 33, 0, 901u, 901u, 1u, 45u),
                RdcDebugStack(765u, 34, 0, 903u, 903u, 1u, 10u),
                RdcDebugStack(765u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(784u, -1, -1, 0u, 0u, 0u, 0u),
                RdcDebugStack(784u, 0, -1, 15u, 15u, 0u, 0u),
                RdcDebugStack(784u, 1, -1, 16u, 16u, 0u, 0u),
                RdcDebugStack(784u, 2, -1, 17u, 17u, 0u, 0u),
                RdcDebugStack(784u, 3, -1, 18u, 18u, 0u, 0u),
            ), frameTracker.frames)
            assertEquals(hashMapOf(0 to 715u, 6 to 732u, 12 to 749u, 25 to 765u, 37 to 784u), frameTracker.drawCallChanges)
            val expectedSourcesFull = listOf(listOf("unnamed_shader"), listOf("unnamed_shader"), listOf("unnamed_shader"), listOf("unnamed_shader"), emptyList())
            assertSourceNamesPerDrawCall(frameTracker, expectedSourcesFull, emptyList())
        }

        private suspend fun assertCaptureThroughDebugWithBreakpoints(modelLifetime: Lifetime, capture: RdcCapture) {
            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.tryDebugVertex.startSuspending(sessionLifetime,
                    RdcDebugVertexInput(0u, 17u,
                        listOf(
                            RdcSourceBreakpoint("Assets/ShaderForSphere.shader", 22u),
                            RdcSourceBreakpoint("Assets/NewShader.shader", 45u),
                            RdcSourceBreakpoint("Assets/NewShader.shader", 59u),
                            RdcSourceBreakpoint("Assets/Cube Shader.shader", 64u),
                            RdcSourceBreakpoint("Assets/mult.hlsl", 3u),
                            RdcSourceBreakpoint("Assets/mult.hlsl", 7u),
                        )))
            }

            val frameTracker = FrameSessionTracker().also { it.init(sessionLifetime, rdDispatcher, debugSession) }
            withContext(rdDispatcher) {
                debugSession.stepOver.fire() // step out from 715
                debugSession.stepOver.fire() // step to 732
                debugSession.stepOver.fire() // on breakpoint in 732
                debugSession.stepOver.fire() // step out from 732
                debugSession.stepOver.fire() // step to 749
                debugSession.stepOver.fire() // on breakpoint in 749
                debugSession.resume.fire()   // on breakpoint in 749
                debugSession.stepOver.fire() // step out from 749
                debugSession.stepOver.fire() // step to 765
                debugSession.stepOver.fire() // step to 784
                debugSession.resume.fire()   // run till the end
            }

            sessionLifetime.waitTermination()
            assertEquals(listOf(
                RdcDebugStack(715u, 22, 0, 885u, 885u, 1u, 10u),
                RdcDebugStack(715u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(732u, -1, -1, 0u, 0u, 0u, 0u),
                RdcDebugStack(732u, 21, 0, 920u, 920u, 1u, 10u),
                RdcDebugStack(732u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(749u, -1, -1, 0u, 0u, 0u, 0u),
                RdcDebugStack(749u, 24, 0, 904u, 904u, 8u, 12u),
                RdcDebugStack(749u, 32, 0, 936u, 936u, 1u, 10u),
                RdcDebugStack(749u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(765u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(784u, -1, -1, 0u, 0u, 0u, 0u),
            ), frameTracker.frames)
            assertEquals(hashMapOf(0 to 715u, 2 to 732u, 5 to 749u, 9 to 765u, 10 to 784u), frameTracker.drawCallChanges)

            val expectedSourcesFull = listOf(listOf("unnamed_shader"), listOf("unnamed_shader"), listOf("unnamed_shader"), listOf("unnamed_shader"), emptyList())
            assertSourceNamesPerDrawCall(frameTracker, expectedSourcesFull, emptyList())
        }

        private suspend fun assertTryDebugVertexStepOver(modelLifetime: Lifetime, capture: RdcCapture, vertId: UInt, breakpoints: List<RdcSourceBreakpoint>, eventsToSkip: List<UInt> = emptyList()) {
            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.tryDebugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(0u, vertId, breakpoints))
            }

            val frameTracker = FrameSessionTracker().also { it.init(sessionLifetime, rdDispatcher, debugSession) }
            withContext(rdDispatcher) {
                // event 715
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()

                //event 732
                debugSession.stepInto.fire()

                // event 749
                debugSession.stepOver.fire()

                // event 765
                debugSession.stepOver.fire()

                // event 784
                debugSession.stepOver.fire()

                debugSession.resume.fire()
            }

            sessionLifetime.waitTermination()
            assertEquals(listOf(
                RdcDebugStack(715u, 0, 0, 883u, 883u, 11u, 45u),
                RdcDebugStack(715u, 19, 0, 883u, 883u, 1u, 45u),
                RdcDebugStack(715u, 20, 0, 884u, 884u, 13u, 28u),
                RdcDebugStack(715u, 21, 0, 884u, 884u, 13u, 34u),
                RdcDebugStack(715u, 22, 0, 885u, 885u, 1u, 10u),
                RdcDebugStack(715u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(732u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(749u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(765u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(784u, -1, -1, 0u, 0u, 0u, 0u),
            ), frameTracker.frames)
            assertEquals(hashMapOf(0 to 715u, 6 to 732u, 7 to 749u, 8 to 765u, 9 to 784u), frameTracker.drawCallChanges)

            val expectedSourcesFull = listOf(listOf("unnamed_shader"), listOf("unnamed_shader"), listOf("unnamed_shader"), listOf("unnamed_shader"), emptyList())
            assertSourceNamesPerDrawCall(frameTracker, expectedSourcesFull, eventsToSkip)
        }

        private suspend fun assertTryDebugUncommonVertexStepByStep(modelLifetime: Lifetime, capture: RdcCapture, breakpoints: List<RdcSourceBreakpoint>) {
            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.tryDebugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(0u, 100u, breakpoints))
            }

            val frameTracker = FrameSessionTracker().also { it.init(sessionLifetime, rdDispatcher, debugSession) }
            withContext(rdDispatcher) {
                // event 715
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()

                debugSession.stepInto.fire()

                // event 732, no vertex 100
                debugSession.stepInto.fire()

                // event 749, no vertex 100
                debugSession.stepInto.fire()

                //event 765
                debugSession.stepInto.fire()
                debugSession.stepInto.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()

                debugSession.stepInto.fire()

                // event 784
                debugSession.stepInto.fire()
                debugSession.stepInto.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.resume.fire()
            }

            sessionLifetime.waitTermination()
            assertEquals(listOf(
                RdcDebugStack(715u, 0, 0, 883u, 883u, 11u, 45u),
                RdcDebugStack(715u, 19, 0, 883u, 883u, 1u, 45u),
                RdcDebugStack(715u, 20, 0, 884u, 884u, 13u, 28u),
                RdcDebugStack(715u, 21, 0, 884u, 884u, 13u, 34u),
                RdcDebugStack(715u, 22, 0, 885u, 885u, 1u, 10u),
                RdcDebugStack(715u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(732u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(749u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(765u, -1, -1, 0u, 0u, 0u, 0u),
                RdcDebugStack(765u, 0, 0, 895u, 895u, 19u, 58u),
                RdcDebugStack(765u, 7, 0, 897u, 897u, 20u, 48u),
                RdcDebugStack(765u, 8, 0, 897u, 897u, 52u, 73u),
                RdcDebugStack(765u, 9, 0, 897u, 897u, 20u, 73u),
                RdcDebugStack(765u, 10, 0, 897u, 897u, 14u, 75u),
                RdcDebugStack(765u, 11, 0, 897u, 897u, 14u, 92u),
                RdcDebugStack(765u, 12, 0, 899u, 899u, 1u, 22u),
                RdcDebugStack(765u, 13, 0, 901u, 901u, 11u, 45u),
                RdcDebugStack(765u, 33, 0, 901u, 901u, 1u, 45u),
                RdcDebugStack(765u, 34, 0, 903u, 903u, 1u, 10u),
                RdcDebugStack(765u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(784u, -1, -1, 0u, 0u, 0u, 0u),
                RdcDebugStack(784u, 0, -1, 15u, 15u, 0u, 0u),
                RdcDebugStack(784u, 1, -1, 16u, 16u, 0u, 0u),
                RdcDebugStack(784u, 2, -1, 17u, 17u, 0u, 0u),
                RdcDebugStack(784u, 3, -1, 18u, 18u, 0u, 0u),
            ), frameTracker.frames)
            assertEquals(hashMapOf(0 to 715u, 6 to 732u, 7 to 749u, 8 to 765u, 20 to 784u), frameTracker.drawCallChanges)

            val expectedSourcesFull = listOf(listOf("unnamed_shader"), listOf("unnamed_shader"), listOf("unnamed_shader"), listOf("unnamed_shader"), emptyList())
            assertSourceNamesPerDrawCall(frameTracker, expectedSourcesFull, listOf(732u, 749u))
        }

        private suspend fun assertTryDebugVertexWithBreakpoints(modelLifetime: Lifetime, capture: RdcCapture, breakpoints: List<RdcSourceBreakpoint>) {
            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.tryDebugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(0u, 17u, breakpoints))
            }

            val frameTracker = FrameSessionTracker().also { it.init(sessionLifetime, rdDispatcher, debugSession) }
            withContext(rdDispatcher) {
                debugSession.resume.fire()
                debugSession.addSourceBreakpoint.fire(RdcSourceBreakpoint("Assets/ShaderForSphere.shader", 22u))
                debugSession.resume.fire()
                debugSession.resume.fire()

                debugSession.removeSourceBreakpoint.fire(RdcSourceBreakpoint("Assets/Cube Shader.shader", 44u))
                debugSession.removeSourceBreakpoint.fire(RdcSourceBreakpoint("Assets/Cube Shader.shader", 62u))
                debugSession.removeSourceBreakpoint.fire(RdcSourceBreakpoint("Assets/Waves.shader", 47u))
                debugSession.addSourceBreakpoint.fire(RdcSourceBreakpoint("Assets/Waves.shader", 53u))
                debugSession.resume.fire()
                debugSession.resume.fire()

                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepInto.fire()

                debugSession.addLineBreakpoint.fire(RdcLineBreakpoint(-1, 22u))
                debugSession.addLineBreakpoint.fire(RdcLineBreakpoint(-1, 167u))
                debugSession.addLineBreakpoint.fire(RdcLineBreakpoint(-1, 125u))
                debugSession.removeLineBreakpoint.fire(RdcLineBreakpoint(-1, 22u))
                debugSession.resume.fire()
                debugSession.resume.fire()
            }

            sessionLifetime.waitTermination()
            assertEquals(listOf(
                RdcDebugStack(715u, 0, 0, 883u, 883u, 11u, 45u),
                RdcDebugStack(715u, 19, 0, 883u, 883u, 1u, 45u),
                RdcDebugStack(715u, 22, 0, 885u, 885u, 1u, 10u),
                RdcDebugStack(732u, 20, 0, 919u, 919u, 13u, 35u),
                RdcDebugStack(749u, 24, 0, 904u, 904u, 8u, 12u),
                RdcDebugStack(765u, 34, 0, 903u, 903u, 1u, 10u),
                RdcDebugStack(765u, -1, -1, 0u, 0u, 0u, 0u),
                RdcDebugStack(784u, -1, -1, 0u, 0u, 0u, 0u),
                RdcDebugStack(784u, 0, -1, 15u, 15u, 0u, 0u),
                RdcDebugStack(784u, 111, -1, 167u, 167u, 0u, 0u)
            ), frameTracker.frames)
            assertEquals(hashMapOf(0 to 715u, 3 to 732u, 4 to 749u, 5 to 765u, 7 to 784u), frameTracker.drawCallChanges)

            val expectedSourcesFull = listOf(listOf("unnamed_shader"), listOf("unnamed_shader"), listOf("unnamed_shader"), listOf("unnamed_shader"), emptyList())
            assertSourceNamesPerDrawCall(frameTracker, expectedSourcesFull, emptyList())
        }

        private suspend fun assertDebugPixelStepByStepDisassembly(modelLifetime: Lifetime, capture: RdcCapture) {
            // instantly finishing sessions
            RenderDocClientTest.assertSessionFinishesImmediately(modelLifetime, capture, RdcDebugPixelInput(0u, 0u, 0u, emptyList()), true)

            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.debugPixel.startSuspending(sessionLifetime, RdcDebugPixelInput(1043u, 1133u, 664u, emptyList()))
            }
            val drawCallSession = debugSession.sessionState.value?.drawCallSession
            assertNotNull(drawCallSession)
            assertTrue(drawCallSession!!.sourceFiles.isEmpty())

            val frameTracker = FrameSessionTracker().also { it.init(sessionLifetime, rdDispatcher, debugSession) }
            withContext(rdDispatcher) {
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepInto.fire()
            }

            sessionLifetime.waitTermination()

            assertEquals(listOf(
                RdcDebugStack(1043u, 0, -1, 10u, 10u, 0u, 0u),
                RdcDebugStack(1043u, 1, -1, 11u, 11u, 0u, 0u),
                RdcDebugStack(1043u, 2, -1, 12u, 12u, 0u, 0u),
                ), frameTracker.frames)
            assertEquals(hashMapOf(0 to 1043u), frameTracker.drawCallChanges)

            val expectedSourcesFull = listOf<List<String>?>(emptyList())
            assertSourceNamesPerDrawCall(frameTracker, expectedSourcesFull, emptyList())
        }

        private suspend fun assertDebugPixelStepByStepShaderLab(modelLifetime: Lifetime, capture: RdcCapture) {
            // instantly finishing sessions
            RenderDocClientTest.assertSessionFinishesImmediately(modelLifetime, capture, RdcDebugPixelInput(739u, 826u, 914u, emptyList()), true)
            RenderDocClientTest.assertSessionFinishesImmediately(modelLifetime, capture, RdcDebugPixelInput(749u, 826u, 914u, emptyList()), true)

            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.debugPixel.startSuspending(sessionLifetime, RdcDebugPixelInput(732u, 1133u, 664u, emptyList()))
            }
            val drawCallSession = debugSession.sessionState.value?.drawCallSession
            assertNotNull(drawCallSession)
            assertEquals("unnamed_shader", drawCallSession!!.sourceFiles[0].name)

            val frameTracker = FrameSessionTracker().also { it.init(sessionLifetime, rdDispatcher, debugSession) }
            withContext(rdDispatcher) {
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepInto.fire()
                debugSession.stepOver.fire()
            }

            sessionLifetime.waitTermination()

            assertEquals(listOf(
                RdcDebugStack(732u, 0, 0, 932u, 932u, 1u, 9u),
                RdcDebugStack(732u, 1, 0, 933u, 933u, 1u, 10u),
                RdcDebugStack(732u, 2, 0, 934u, 934u, 28u, 40u),
                RdcDebugStack(732u, 4, 0, 934u, 934u, 8u, 48u),
                RdcDebugStack(732u, 7, 0, 934u, 934u, 1u, 50u)
            ), frameTracker.frames)
            assertEquals(hashMapOf(0 to 732u), frameTracker.drawCallChanges)

            val expectedSourcesFull = listOf(listOf("unnamed_shader"))
            assertSourceNamesPerDrawCall(frameTracker, expectedSourcesFull, emptyList())
        }

        private suspend fun assertTryDebugPixelStepByStep(modelLifetime: Lifetime, capture: RdcCapture, breakpoints: List<RdcSourceBreakpoint>) {
            // instantly finishing sessions
            RenderDocClientTest.assertSessionFinishesImmediately(modelLifetime, capture, RdcDebugPixelInput(0u, 0u, 0u, breakpoints), false)
            RenderDocClientTest.assertSessionFinishesImmediately(modelLifetime, capture, RdcDebugPixelInput(0u, 826u, 914u, breakpoints), false)

            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.tryDebugPixel.startSuspending(sessionLifetime, RdcDebugPixelInput(0u, 914u, 534u, breakpoints))
            }

            val frameTracker = FrameSessionTracker().also { it.init(sessionLifetime, rdDispatcher, debugSession) }
            withContext(rdDispatcher) {
                // event 749
                debugSession.stepInto.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()

                debugSession.stepOver.fire()

                // event 765
                debugSession.stepInto.fire()
                debugSession.stepOver.fire()

                debugSession.stepOver.fire()

                // event 784
                debugSession.stepInto.fire()
                repeat(17) { debugSession.stepOver.fire() }

                debugSession.stepInto.fire()

                // event 811
                debugSession.stepInto.fire()
                repeat(18) { debugSession.stepOver.fire() }
                debugSession.stepOver.fire()

                // event 824 should be skipped, no (914, 534) pixel in the event
                debugSession.stepInto.fire()
                debugSession.resume.fire()
            }

            sessionLifetime.waitTermination()
            assertEquals(listOf(
                RdcDebugStack(749u, 1, 0, 944u, 944u, 8u, 23u),
                RdcDebugStack(749u, 4, 0, 900u, 900u, 8u, 12u),
                RdcDebugStack(749u, 6, 0, 944u, 944u, 27u, 50u),
                RdcDebugStack(749u, 7, 0, 944u, 944u, 8u, 50u),
                RdcDebugStack(749u, 8, 0, 944u, 944u, 1u, 52u),
                RdcDebugStack(749u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(765u, -1, -1, 0u, 0u, 0u, 0u),
                RdcDebugStack(765u, 0, 0, 908u, 908u, 1u, 15u),
                RdcDebugStack(765u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(784u, -1, -1, 0u, 0u, 0u, 0u),
                RdcDebugStack(784u, 0, -1, 12u, 12u, 0u, 0u),
                RdcDebugStack(784u, 1, -1, 13u, 13u, 0u, 0u),
                RdcDebugStack(784u, 2, -1, 14u, 14u, 0u, 0u),
                RdcDebugStack(784u, 3, -1, 15u, 15u, 0u, 0u),
                RdcDebugStack(784u, 4, -1, 16u, 16u, 0u, 0u),
                RdcDebugStack(784u, 5, -1, 17u, 17u, 0u, 0u),
                RdcDebugStack(784u, 6, -1, 18u, 18u, 0u, 0u),
                RdcDebugStack(784u, 7, -1, 19u, 19u, 0u, 0u),
                RdcDebugStack(784u, 8, -1, 20u, 20u, 0u, 0u),
                RdcDebugStack(784u, 9, -1, 21u, 21u, 0u, 0u),
                RdcDebugStack(784u, 10, -1, 22u, 22u, 0u, 0u),
                RdcDebugStack(784u, 11, -1, 23u, 23u, 0u, 0u),
                RdcDebugStack(784u, 12, -1, 24u, 24u, 0u, 0u),
                RdcDebugStack(784u, 13, -1, 25u, 25u, 0u, 0u),
                RdcDebugStack(784u, 14, -1, 26u, 26u, 0u, 0u),
                RdcDebugStack(784u, 15, -1, 27u, 27u, 0u, 0u),
                RdcDebugStack(784u, 16, -1, 28u, 28u, 0u, 0u),
                RdcDebugStack(784u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(811u, -1, -1, 0u, 0u, 0u, 0u),
                RdcDebugStack(811u, 0, -1, 10u, 10u, 0u, 0u),
                RdcDebugStack(811u, 1, -1, 11u, 11u, 0u, 0u),
                RdcDebugStack(811u, 2, -1, 12u, 12u, 0u, 0u),
                RdcDebugStack(811u, 3, -1, 13u, 13u, 0u, 0u),
                RdcDebugStack(811u, 4, -1, 14u, 14u, 0u, 0u),
                RdcDebugStack(811u, 5, -1, 15u, 15u, 0u, 0u),
                RdcDebugStack(811u, 6, -1, 16u, 16u, 0u, 0u),
                RdcDebugStack(811u, 7, -1, 17u, 17u, 0u, 0u),
                RdcDebugStack(811u, 8, -1, 18u, 18u, 0u, 0u),
                RdcDebugStack(811u, 9, -1, 19u, 19u, 0u, 0u),
                RdcDebugStack(811u, 10, -1, 20u, 20u, 0u, 0u),
                RdcDebugStack(811u, 11, -1, 21u, 21u, 0u, 0u),
                RdcDebugStack(811u, 12, -1, 22u, 22u, 0u, 0u),
                RdcDebugStack(811u, 13, -1, 23u, 23u, 0u, 0u),
                RdcDebugStack(811u, 14, -1, 24u, 24u, 0u, 0u),
                RdcDebugStack(811u, 15, -1, 25u, 25u, 0u, 0u),
                RdcDebugStack(811u, 16, -1, 26u, 26u, 0u, 0u),
                RdcDebugStack(811u, 17, -1, 27u, 27u, 0u, 0u),
                RdcDebugStack(811u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(824u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(837u, -1, -1, 0u, 0u, 0u, 0u),
            ), frameTracker.frames)
            assertEquals(hashMapOf(0 to 749u, 6 to 765u, 9 to 784u, 28 to 811u, 48 to 824u, 49 to 837u), frameTracker.drawCallChanges)

            val expectedSourcesFull = listOf(listOf("unnamed_shader"), listOf("unnamed_shader"), emptyList(), emptyList(), null, null)
            assertSourceNamesPerDrawCall(frameTracker, expectedSourcesFull, listOf(824u, 837u))
        }

        private suspend fun assertTryDebugPixelWithBreakpoints(modelLifetime: Lifetime, capture: RdcCapture, breakpoints: List<RdcSourceBreakpoint>) {
            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.tryDebugPixel.startSuspending(sessionLifetime, RdcDebugPixelInput(0u, 914u, 535u,
                    breakpoints + listOf(RdcSourceBreakpoint("Assets/ShaderForSphere.shader", 27u))))
            }

            val frameTracker = FrameSessionTracker().also { it.init(sessionLifetime, rdDispatcher, debugSession) }
            withContext(rdDispatcher) {
                debugSession.resume.fire()
                debugSession.addSourceBreakpoint.fire(RdcSourceBreakpoint("Assets/Cube Shader.shader", 69u))
                debugSession.resume.fire()
                debugSession.removeSourceBreakpoint.fire(RdcSourceBreakpoint("Assets/Cube Shader.shader", 72u))
                debugSession.removeSourceBreakpoint.fire(RdcSourceBreakpoint("Assets/mult.hlsl", 3u))
                debugSession.resume.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepInto.fire()

                debugSession.addLineBreakpoint.fire(RdcLineBreakpoint(-1, 15u))
                debugSession.addLineBreakpoint.fire(RdcLineBreakpoint(-1, 28u))
                debugSession.removeLineBreakpoint.fire(RdcLineBreakpoint(-1, 15u))
                debugSession.resume.fire()
                debugSession.resume.fire()
            }

            sessionLifetime.waitTermination()
            assertEquals(listOf(
                RdcDebugStack(715u, 0, 0, 890u, 890u, 8u, 30u),
                RdcDebugStack(715u, 1, 0, 890u, 890u, 1u, 32u),

                RdcDebugStack(749u, 0, 0, 941u, 943u, 13u, 1u),

                RdcDebugStack(765u, 0, 0, 908u, 908u, 1u, 15u),
                RdcDebugStack(765u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(784u, -1, -1, 0u, 0u, 0u, 0u),
                RdcDebugStack(784u, 0, -1, 12u, 12u, 0u, 0u),
                RdcDebugStack(784u, 16, -1, 28u, 28u, 0u, 0u)
            ), frameTracker.frames)
            assertEquals(hashMapOf(0 to 715u, 2 to 749u, 3 to 765u, 5 to 784u), frameTracker.drawCallChanges)

            val expectedSourcesFull = listOf(listOf("unnamed_shader"), listOf("unnamed_shader"), listOf("unnamed_shader"), emptyList())
            assertSourceNamesPerDrawCall(frameTracker, expectedSourcesFull, emptyList())
        }

        private suspend fun assertTryDebugPixelStepOver(modelLifetime: Lifetime, capture: RdcCapture, breakpoints: List<RdcSourceBreakpoint>) {
            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.tryDebugPixel.startSuspending(sessionLifetime, RdcDebugPixelInput(0u, 914u, 534u, breakpoints))
            }

            val frameTracker = FrameSessionTracker().also { it.init(sessionLifetime, rdDispatcher, debugSession) }
            withContext(rdDispatcher) {
                // event 715
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()

                debugSession.stepOver.fire()
                debugSession.stepInto.fire() // try step into 732, should skip

                // event 749
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()

                debugSession.stepOver.fire()

                // event 765
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()

                for (eventId in listOf(784, 811, 824, 837)) {
                    // skip these events, should not go into
                    debugSession.stepOver.fire()
                }
                debugSession.resume.fire()
            }

            sessionLifetime.waitTermination()

            assertEquals(listOf(
                RdcDebugStack(715u, 0, 0, 890u, 890u, 8u, 30u),
                RdcDebugStack(715u, 1, 0, 890u, 890u, 1u, 32u),
                RdcDebugStack(715u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(732u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(749u, -1, -1, 0u, 0u, 0u, 0u),
                RdcDebugStack(749u, 1, 0, 944u, 944u, 8u, 23u),
                RdcDebugStack(749u, 4, 0, 900u, 900u, 8u, 12u),
                RdcDebugStack(749u, 6, 0, 944u, 944u, 27u, 50u),
                RdcDebugStack(749u, 7, 0, 944u, 944u, 8u, 50u),
                RdcDebugStack(749u, 8, 0, 944u, 944u, 1u, 52u),
                RdcDebugStack(749u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(765u, -1, -1, 0u, 0u, 0u, 0u),
                RdcDebugStack(765u, 0, 0, 908u, 908u, 1u, 15u),
                RdcDebugStack(765u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(784u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(811u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(824u, -1, -1, 0u, 0u, 0u, 0u),

                RdcDebugStack(837u, -1, -1, 0u, 0u, 0u, 0u),
            ), frameTracker.frames)
            assertEquals(hashMapOf(0 to 715u, 3 to 732u, 4 to 749u, 11 to 765u, 14 to 784u, 15 to 811u, 16 to 824u, 17 to 837u), frameTracker.drawCallChanges)

            val expectedSourcesFull = listOf(listOf("unnamed_shader"), null, listOf("unnamed_shader"), listOf("unnamed_shader"), emptyList(), emptyList(), null, null)
            assertSourceNamesPerDrawCall(frameTracker, expectedSourcesFull, listOf(732u, 824u, 837u))
        }

        private suspend fun assertVerticesTable(modelLifetime: Lifetime, capture: RdcCapture) {
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher

            run {
                val vertices = withContext(rdDispatcher) {
                    capture.getVertexStageInOutputs.startSuspending(modelLifetime, 787)
                }
                assertEquals(null, vertices)
            }

            run {
                val vertices = withContext(rdDispatcher) {
                    capture.getVertexStageInOutputs.startSuspending(modelLifetime, 97)
                }
                assertEquals(null, vertices)
            }

            run {
                val vertices = withContext(rdDispatcher) {
                    capture.getVertexStageInOutputs.startSuspending(modelLifetime, 112)
                }
                assertNotEquals(null, vertices)
                assertEquals(listOf(22561u, 22563u, 22562u, 22562u, 22563u, 22564u), vertices?.input_indices)
                assertEquals(listOf("POSITION", "COLOR", "TEXCOORD0"),vertices?.input_columns)
                assertEquals(listOf("SV_POSITION", "COLOR", "TEXCOORD0", "TEXCOORD1"), vertices?.output_columns)
                assertEquals(
                    listOf(
                        listOf(-6.713867E-4f, 761.333f, 0.00f),
                        listOf(0.09803922f, 0.09803922f, 0.09803922f, 1.00f),
                        listOf(0.00f, 0.00f)
                    ), vertices?.inputs?.get(0))

                assertEquals(
                    listOf(
                        listOf(1356.6663f, 0.00f, 0.00f),
                        listOf(0.09803922f, 0.09803922f, 0.09803922f, 1.00f),
                        listOf(1.00f, 1.00f)
                    ), vertices?.inputs?.get(5))

                assertEquals(
                    listOf(
                        listOf(0.9999999f, -1.00f, 0.990099f, 1.00f),
                        listOf(0.09803922f, 0.09803922f, 0.09803922f, 1.00f),
                        listOf(1.00f, 0.00f),
                        listOf(0.93749994f, 0.93750006f)
                    ), vertices?.outputs?.get(2))

                assertEquals(
                    listOf(
                        listOf(0.9999999f, -1.00f, 0.990099f, 1.00f),
                        listOf(0.09803922f, 0.09803922f, 0.09803922f, 1.00f),
                        listOf(1.00f, 0.00f),
                        listOf(0.93749994f, 0.93750006f)
                    ), vertices?.outputs?.get(3))
            }

            run {
                val vertices = withContext(rdDispatcher) {
                    capture.getVertexStageInOutputs.startSuspending(modelLifetime, 677)
                }
                assertNotEquals(null, vertices)
                assertEquals(listOf(0u, 1u, 2u), vertices?.input_indices)
                assertEquals(listOf(0u, 1u, 2u), vertices?.output_indices)
                assertTrue(vertices?.input_columns?.isEmpty() ?: false)
                assertEquals(listOf("SV_POSITION", "TEXCOORD"), vertices?.output_columns)
                assertEquals(listOf<List<Float>>(emptyList(), emptyList(), emptyList()), vertices?.inputs)
                assertEquals(listOf(
                    listOf(listOf(-1.0f, -1.0f, 1.0f, 1.0f), listOf(0.0f, 1.0f)),
                    listOf(listOf(3.0f, -1.0f, 1.0f, 1.0f), listOf(2.0f, 1.0f)),
                    listOf(listOf(-1.0f, 3.0f, 1.0f, 1.0f), listOf(0.0f, -1.0f))
                ), vertices?.outputs)
            }

            run {
                val vertices = withContext(rdDispatcher) {
                    capture.getVertexStageInOutputs.startSuspending(modelLifetime, 715)
                }
                assertNotEquals(null, vertices)
                assertEquals(2304, vertices?.input_indices?.size)
                assertEquals(2304, vertices?.output_indices?.size)
                assertEquals(vertices?.input_indices, vertices?.output_indices)
                assertEquals(177u, vertices?.input_indices?.get(0))
                assertEquals(386u, vertices?.input_indices?.get(1))
                assertEquals(298u, vertices?.input_indices?.get(1442))
                assertEquals(510u, vertices?.input_indices?.get(2303))

                assertEquals(listOf("POSITION", "NORMAL", "TEXCOORD0"), vertices?.input_columns)
                assertEquals(listOf("SV_POSITION", "COLOR"), vertices?.output_columns)

                assertEquals(2304, vertices?.inputs?.size)
                assertEquals(2304, vertices?.outputs?.size)

                assertEquals(listOf(
                    listOf(0.19199951f, -0.45026365f, -0.09450744f),
                    listOf(0.3828188f, -0.9043397f, -0.18873115f),
                    listOf(0.4259613f, 0.17526756f)
                ), vertices?.inputs?.get(0))

                assertEquals(listOf(
                    listOf(0.090518795f, 0.27555177f, -0.40696776f),
                    listOf(0.18049115f, 0.5502449f, -0.81526285f),
                    listOf(0.28369975f, 0.6757817f)
                ), vertices?.inputs?.get(2227))

                assertEquals(listOf(
                    listOf(-0.28883544f, 0.2879004f, -0.28883544f),
                    listOf(-0.5770534f, 0.5779435f, -0.5770534f),
                    listOf(0.12398553f, 0.68360907f)
                ), vertices?.inputs?.get(2303))

                assertEquals(listOf(
                    listOf(-0.89212215f, 0.017701358f, 0.0561663f, 5.593367f),
                    listOf(0.6914094f, 0.047830164f, 0.40563443f)
                ), vertices?.outputs?.get(0))

                assertEquals(listOf(
                    listOf(-0.8177031f, -0.5423268f, 0.056164134f, 6.0209064f),
                    listOf(0.21132487f, 0.21132487f, 0.21132484f)
                ), vertices?.outputs?.get(227))

                assertEquals(listOf(
                    listOf(-0.6705234f, -1.33369f, 0.056168433f, 5.1719327f),
                    listOf(0.6834294f, 0.9274427f, 0.6834294f)
                ), vertices?.outputs?.get(524))
            }

            run {
                val vertices = withContext(rdDispatcher) {
                    capture.getVertexStageInOutputs.startSuspending(modelLifetime, 765)
                }
                assertNotEquals(null, vertices)
                val verticesGrouped = withContext(rdDispatcher) {
                    capture.getVertexStageInOutputs.startSuspending(modelLifetime, 739)
                }
                assertEquals(vertices, verticesGrouped)
                assertEquals(600, vertices?.input_indices?.size)
                assertEquals(600, vertices?.output_indices?.size)
                assertEquals(vertices?.input_indices, vertices?.output_indices)
                assertEquals(9u, vertices?.input_indices?.get(0))
                assertEquals(21u, vertices?.input_indices?.get(1))
                assertEquals(59u, vertices?.input_indices?.get(526))
                assertEquals(60u, vertices?.input_indices?.get(530))

                assertEquals(listOf("POSITION", "NORMAL"), vertices?.input_columns)
                assertEquals(listOf("SV_POSITION"), vertices?.output_columns)

                assertEquals(600, vertices?.inputs?.size)
                assertEquals(600, vertices?.outputs?.size)

                assertEquals(listOf(
                    listOf(-4.0000005f, -1.110223E-16f, 5f),
                    listOf(0f, 1f, 0f)
                ), vertices?.inputs?.get(0))

                assertEquals(listOf(
                    listOf(0f, -6.6613384E-17f, 3f),
                    listOf(0f, 1f, 0f),
                ), vertices?.inputs?.get(123))

                assertEquals(listOf(
                    listOf(0.99999994f, -6.6613384E-17f, 3f),
                    listOf(0f, 1f, 0f)
                ), vertices?.inputs?.get(599))

                assertEquals(listOf(
                    listOf(5.9477596f, 0.72631633f, 0.056128737f, 13.007547f),
                ), vertices?.outputs?.get(0))

                assertEquals(listOf(
                    listOf(2.5044744f, 3.0240726f, 0.056139242f, 10.933749f),
                ), vertices?.outputs?.get(123))

                assertEquals(listOf(
                    listOf(2.034446f, 2.7630925f, 0.05614412f, 9.971612f),
                ), vertices?.outputs?.get(599))
            }
        }

        private suspend fun assertTexturesOutputs(modelLifetime: Lifetime, capture: RdcCapture) {
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher

            run {
                val outputsRoot = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, -1)!!
                }

                val outputsLast = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 1671)!!
                }
                assertEquals(outputsRoot, outputsLast)
                assertEquals(1, outputsRoot.colorOutputs.size)
                val colorOutput = outputsRoot.colorOutputs[0]
                assertEquals("Swapchain Image 11155", colorOutput.name)
                assertEquals(2035, colorOutput.width)
                assertEquals(1142, colorOutput.height)

                assertNull(outputsRoot.depthOutput)
            }

            run {
                val outputsGrouped = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 97)!!
                }
                val outputsLeaf = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 1638)!!
                }
                assertEquals(outputsGrouped, outputsLeaf)
                assertEquals(1, outputsGrouped.colorOutputs.size)
                val colorOutput = outputsGrouped.colorOutputs[0]
                assertEquals("GUIViewHDRRT", colorOutput.name)
                assertEquals(2035, colorOutput.width)
                assertEquals(1142, colorOutput.height)

                assertNotNull(outputsGrouped.depthOutput)
                val depthOutput = outputsGrouped.depthOutput!!
                assertEquals("GUIViewHDRRT", depthOutput.name)
                assertEquals(2035, depthOutput.width)
                assertEquals(1142, depthOutput.height)
            }

            run {
                val outputs = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 0)!!
                }
                assertEquals(1, outputs.colorOutputs.size)
                val colorOutput = outputs.colorOutputs[0]
                assertEquals("Swapchain Image 11155", colorOutput.name)
                assertEquals(2035, colorOutput.width)
                assertEquals(1142, colorOutput.height)

                assertNotNull(outputs.depthOutput)
                val depthOutput = outputs.depthOutput!!
                assertEquals("RenderTexture-2D-2035x1142", depthOutput.name)
                assertEquals(2035, depthOutput.width)
                assertEquals(1142, depthOutput.height)
            }

            run {
                val outputsGrouped = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 543)!!
                }
                assertEquals(1, outputsGrouped.colorOutputs.size)
                val colorOutput = outputsGrouped.colorOutputs[0]
                assertEquals("SceneView RT", colorOutput.name)
                assertEquals(2032, colorOutput.width)
                assertEquals(1070, colorOutput.height)

                assertNotNull(outputsGrouped.depthOutput)
                val depthOutput = outputsGrouped.depthOutput!!
                assertEquals("SceneView RT", depthOutput.name)
                assertEquals(2032, depthOutput.width)
                assertEquals(1070, depthOutput.height)
            }

            val outputsGrouped = withContext(rdDispatcher) {
                capture.getTextureRGBBuffer.startSuspending(modelLifetime, 696)!!
            }

            run {
                val outputsGrouped1 = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 739)!!
                }
                assertEquals(outputsGrouped, outputsGrouped1)
                val colorOutput = outputsGrouped1.colorOutputs[0]
                assertEquals("_CameraColorAttachmentA_2032x1070_R16G16B16A16_SFloat_Tex2D", colorOutput.name)
                assertEquals(2032, colorOutput.width)
                assertEquals(1070, colorOutput.height)

                assertNotNull(outputsGrouped1.depthOutput)
                val depthOutput = outputsGrouped1.depthOutput!!
                assertEquals("_CameraDepthAttachment_2032x1070_Depth", depthOutput.name)
                assertEquals(2032, depthOutput.width)
                assertEquals(1070, depthOutput.height)
            }

            run {
                val outputsLeaf = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 715)!!
                }
                assertEquals(1, outputsLeaf.colorOutputs.size)
                val colorOutput = outputsLeaf.colorOutputs[0]
                assertEquals("_CameraColorAttachmentA_2032x1070_R16G16B16A16_SFloat_Tex2D", colorOutput.name)
                assertEquals(2032, colorOutput.width)
                assertEquals(1070, colorOutput.height)

                assertNotNull(outputsLeaf.depthOutput)
                val depthOutput = outputsLeaf.depthOutput!!
                assertEquals("_CameraDepthAttachment_2032x1070_Depth", depthOutput.name)
                assertEquals(2032, depthOutput.width)
                assertEquals(1070, depthOutput.height)
            }

            run {
                val outputsGrouped1 = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 697)!!
                }
                assertNotEquals(outputsGrouped1, outputsGrouped)
                assertEquals(1, outputsGrouped1.colorOutputs.size)
                val colorOutput = outputsGrouped1.colorOutputs[0]
                assertEquals("_CameraColorAttachmentA_2032x1070_R16G16B16A16_SFloat_Tex2D", colorOutput.name)
                assertEquals(2032, colorOutput.width)
                assertEquals(1070, colorOutput.height)

                assertNotNull(outputsGrouped1.depthOutput)
                val depthOutput = outputsGrouped1.depthOutput!!
                assertEquals("_CameraDepthAttachment_2032x1070_Depth", depthOutput.name)
                assertEquals(2032, depthOutput.width)
                assertEquals(1070, depthOutput.height)

                val outputsEndEvent = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 738)
                }
                assertEquals(outputsGrouped1, outputsEndEvent)
            }

            run {
                val outputsLeaf = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 1244)!!
                }
                assertEquals(1, outputsLeaf.colorOutputs.size)
                val colorOutput = outputsLeaf.colorOutputs[0]
                assertEquals("SceneView RT", colorOutput.name)
                assertEquals(2032, colorOutput.width)
                assertEquals(1070, colorOutput.height)

                assertNotNull(outputsLeaf.depthOutput)
                val depthOutput = outputsLeaf.depthOutput!!
                assertEquals("SceneView RT", depthOutput.name)
                assertEquals(2032, depthOutput.width)
                assertEquals(1070, depthOutput.height)
            }
        }

        suspend fun testRenderDocClient(lifetime: Lifetime, capture: RdcCapture) {
            // The following draw calls use user's shader files:
            // 715 - Assets/ShaderForSphere.shader
            // 732 - Assets/NewShader.shader
            // 749 - Assets/Cube Shader.shader, Assets/mult.hlsl
            // 765 - Assets/Waves.shader

            val breakpoints = mutableListOf(
                RdcSourceBreakpoint("Assets/Cube Shader.shader", 44u),
                RdcSourceBreakpoint("Assets/Cube Shader.shader", 62u),
                RdcSourceBreakpoint("Assets/Cube Shader.shader", 72u),
                RdcSourceBreakpoint("Assets/NewShader.shader", 44u),
                RdcSourceBreakpoint("Assets/NewShader.shader", 59u),
                RdcSourceBreakpoint("Assets/ShaderForSphere.shader", 20u),
                RdcSourceBreakpoint("Assets/mult.hlsl", 3u),
                RdcSourceBreakpoint("Assets/mult.hlsl", 7u),
            )

            assertActionsCollection(capture)
            assertDebugVertexStepByStepDisassembly(lifetime, capture)
            assertDebugVertexStepByStepShaderLab(lifetime, capture)
            assertDebugVertexWithBreakpoints(lifetime, capture)

            assertTryDebugVertexStepOver(lifetime, capture, 35u, listOf(RdcSourceBreakpoint("Assets/ShaderForSphere.shader", 20u)), listOf(811u))
            assertTryDebugVertexStepOver(lifetime, capture, 100u, breakpoints, listOf(732u, 749u, 811u))
            breakpoints += listOf(RdcSourceBreakpoint("Assets/Waves.shader", 47u), RdcSourceBreakpoint("Assets/Waves.shader", 58u))

            assertTryDebugVertexStepByStep(lifetime, capture, breakpoints)
            assertTryDebugUncommonVertexStepByStep(lifetime, capture, breakpoints)
            assertTryDebugVertexWithBreakpoints(lifetime, capture, breakpoints)
            assertCaptureThroughDebugWithBreakpoints(lifetime, capture)

            assertDebugPixelStepByStepDisassembly(lifetime, capture)
            assertDebugPixelStepByStepShaderLab(lifetime, capture)
            assertTryDebugPixelStepByStep(lifetime, capture, breakpoints)

            breakpoints.add(RdcSourceBreakpoint("Assets/ShaderForSphere.shader", 27u))
            assertTryDebugPixelWithBreakpoints(lifetime, capture, breakpoints)
            assertTryDebugPixelStepOver(lifetime, capture, breakpoints)

            assertVerticesTable(lifetime, capture)
            assertTexturesOutputs(lifetime, capture)
        }
    }
}
