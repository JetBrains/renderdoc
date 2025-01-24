import com.jetbrains.rd.framework.protocolOrThrow
import com.jetbrains.rd.util.lifetime.Lifetime
import com.jetbrains.rd.util.lifetime.waitTermination
import com.jetbrains.rd.util.reactive.fire
import com.jetbrains.rd.util.reactive.valueOrThrow
import com.jetbrains.rd.util.threading.coroutines.adviseSuspend
import com.jetbrains.rd.util.threading.coroutines.asCoroutineDispatcher
import com.jetbrains.renderdoc.rdClient.model.*
import kotlinx.coroutines.*
import org.junit.jupiter.api.Assertions.*
import kotlin.io.path.Path
import kotlin.io.path.name


class RenderDocClientWindowsTest {
    companion object {
        private suspend fun assertDebugVertexStepByStepDisassembly(modelLifetime: Lifetime, capture: RdcCapture) {
            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.debugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(784u, 5039u, emptyList()))
            }
            assertTrue(debugSession.drawCallSession.valueOrThrow.sourceFiles.isEmpty())

            val expectedLineNumbers = mutableListOf(15u)
            val lineNumbers = mutableListOf<UInt>()
            withContext(rdDispatcher) {
                debugSession.currentStack.adviseSuspend(sessionLifetime, rdDispatcher) {
                    if (it != null) {
                        lineNumbers.add(it.lineStart)
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

        private suspend fun assertDebugVertexStepByStepShaderLab(modelLifetime: Lifetime, capture: RdcCapture) {
            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.debugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(732u, 0u, emptyList()))
            }
            assertEquals(debugSession.drawCallSession.valueOrThrow.sourceFiles[0].name, "unnamed_shader")

            val frames = mutableListOf<RdcDebugStack>()
            withContext(rdDispatcher) {
                debugSession.currentStack.adviseSuspend(sessionLifetime, rdDispatcher) {
                    if (it != null) {
                        frames.add(it)
                    } else {
                        sessionLifetime.terminate()
                    }
                }
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
                ), frames
            )
        }

        private suspend fun assertTryDebugVertexStepByStep(modelLifetime: Lifetime, capture: RdcCapture, breakpoints: List<RdcSourceBreakpoint>) {
            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.tryDebugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(0u, 0u, breakpoints))
            }

            val frames = mutableListOf<RdcDebugStack>()
            withContext(rdDispatcher) {
                debugSession.currentStack.adviseSuspend(sessionLifetime, rdDispatcher) {
                    if (it != null) {
                        frames.add(it)
                    } else {
                        sessionLifetime.terminate()
                    }
                }

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

                //skip 749
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()

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
            ), frames)
        }

        private suspend fun assertTryDebugVertexWithBreakpoints(modelLifetime: Lifetime, capture: RdcCapture, breakpoints: List<RdcSourceBreakpoint>) {
            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.tryDebugVertex.startSuspending(sessionLifetime, RdcDebugVertexInput(0u, 0u, breakpoints))
            }

            val frames = mutableListOf<RdcDebugStack>()
            withContext(rdDispatcher) {
                debugSession.currentStack.adviseSuspend(sessionLifetime, rdDispatcher) {
                    if (it != null) {
                        frames.add(it)
                    } else {
                        sessionLifetime.terminate()
                    }
                }

                debugSession.resume.fire()
                debugSession.addSourceBreakpoint.fire(RdcSourceBreakpoint("Assets/ShaderForSphere.shader", 22u))
                debugSession.resume.fire()
                debugSession.resume.fire()

                debugSession.removeSourceBreakpoint.fire(RdcSourceBreakpoint("Assets/Cube Shader.shader", 44u))
                debugSession.removeSourceBreakpoint.fire(RdcSourceBreakpoint("Assets/Cube Shader.shader", 62u))
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
            ), frames)
        }

        private suspend fun assertDebugPixelStepByStepDisassembly(modelLifetime: Lifetime, capture: RdcCapture) {
            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.debugPixel.startSuspending(sessionLifetime, RdcDebugPixelInput(1043u, 1133u, 664u, emptyList()))
            }
            assertTrue(debugSession.drawCallSession.valueOrThrow.sourceFiles.isEmpty())

            val frames = mutableListOf<RdcDebugStack>()
            withContext(rdDispatcher) {
                debugSession.currentStack.adviseSuspend(sessionLifetime, rdDispatcher) {
                    if (it != null) {
                        frames.add(it)
                    } else {
                        sessionLifetime.terminate()
                    }
                }

                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepInto.fire()
            }

            sessionLifetime.waitTermination()

            assertEquals(listOf(
                RdcDebugStack(1043u, 0, -1, 10u, 10u, 0u, 0u),
                RdcDebugStack(1043u, 1, -1, 11u, 11u, 0u, 0u),
                RdcDebugStack(1043u, 2, -1, 12u, 12u, 0u, 0u),
                ), frames)
        }

        private suspend fun assertDebugPixelStepByStepShaderLab(modelLifetime: Lifetime, capture: RdcCapture) {
            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.debugPixel.startSuspending(sessionLifetime, RdcDebugPixelInput(732u, 1133u, 664u, emptyList()))
            }
            assertEquals(debugSession.drawCallSession.valueOrThrow.sourceFiles[0].name, "unnamed_shader")

            val frames = mutableListOf<RdcDebugStack>()
            withContext(rdDispatcher) {
                debugSession.currentStack.adviseSuspend(sessionLifetime, rdDispatcher) {
                    if (it != null) {
                        frames.add(it)
                    } else {
                        sessionLifetime.terminate()
                    }
                }
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
            ), frames)
        }

        private suspend fun assertTryDebugPixelStepByStep(modelLifetime: Lifetime, capture: RdcCapture, breakpoints: List<RdcSourceBreakpoint>) {
            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.tryDebugPixel.startSuspending(sessionLifetime, RdcDebugPixelInput(0u, 921u, 541u, breakpoints))
            }

            val frames = mutableListOf<RdcDebugStack>()
            withContext(rdDispatcher) {
                debugSession.currentStack.adviseSuspend(sessionLifetime, rdDispatcher) {
                    if (it != null) {
                        frames.add(it)
                    } else {
                        sessionLifetime.terminate()
                    }
                }

                // event 749
                debugSession.stepInto.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()

                debugSession.stepOver.fire()

                //event 765
                debugSession.stepInto.fire()
                debugSession.stepOver.fire()

                debugSession.stepOver.fire()

                //event 784
                debugSession.stepInto.fire()
                debugSession.stepInto.fire()
                debugSession.stepOver.fire()
                debugSession.stepOver.fire()
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
                RdcDebugStack(784u, 3, -1, 15u, 15u, 0u, 0u)

            ), frames)
        }

        private suspend fun assertTryDebugPixelWithBreakpoints(modelLifetime: Lifetime, capture: RdcCapture, breakpoints: List<RdcSourceBreakpoint>) {
            val sessionLifetime = modelLifetime.createNested()
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val debugSession = withContext(rdDispatcher) {
                capture.tryDebugPixel.startSuspending(sessionLifetime, RdcDebugPixelInput(0u, 914u, 535u,
                    breakpoints + listOf(RdcSourceBreakpoint("Assets/ShaderForSphere.shader", 27u))))
            }

            val frames = mutableListOf<RdcDebugStack>()
            withContext(rdDispatcher) {
                debugSession.currentStack.adviseSuspend(sessionLifetime, rdDispatcher) {
                    if (it != null) {
                        frames.add(it)
                    } else {
                        sessionLifetime.terminate()
                    }
                }

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
            ), frames)
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
        }

        private suspend fun assertTexturesOutputs(modelLifetime: Lifetime, capture: RdcCapture) {
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher

            val outputsRoot = withContext(rdDispatcher) {
                capture.getTextureRGBBuffer.startSuspending(modelLifetime, -1)
            }

            run {
                val outputsLast = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 1671)
                }
                assertEquals(outputsRoot, outputsLast)
                assertEquals(1, outputsRoot?.colorOutputs?.size)
                assertNull(outputsRoot?.depthOutput)
                assertEquals(2035, outputsRoot?.colorOutputs?.get(0)?.width)
                assertEquals(1142, outputsRoot?.colorOutputs?.get(0)?.height)
            }

            run {
                val outputsGrouped = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 97)
                }
                val outputsLeaf = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 1638)
                }
                assertEquals(outputsGrouped, outputsLeaf)
                assertEquals(1, outputsGrouped?.colorOutputs?.size)
                assertNotNull(outputsGrouped?.depthOutput)
                assertEquals(2035, outputsRoot?.colorOutputs?.get(0)?.width)
                assertEquals(1142, outputsRoot?.colorOutputs?.get(0)?.height)
            }

            run {
                val outputs = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 0)
                }
                assertEquals(1, outputs?.colorOutputs?.size)
                assertNotNull(outputs?.depthOutput)
                assertEquals(2035, outputs?.colorOutputs?.get(0)?.width)
                assertEquals(1142, outputs?.colorOutputs?.get(0)?.height)
            }

            run {
                val outputsGrouped = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 543)
                }
                assertEquals(1, outputsGrouped?.colorOutputs?.size)
                assertNotNull(outputsGrouped?.depthOutput)
                assertEquals(2032, outputsGrouped?.colorOutputs?.get(0)?.width)
                assertEquals(1070, outputsGrouped?.colorOutputs?.get(0)?.height)
            }

            val outputsGrouped = withContext(rdDispatcher) {
                capture.getTextureRGBBuffer.startSuspending(modelLifetime, 696)
            }

            run {
                val outputsGrouped1 = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 739)
                }
                assertEquals(outputsGrouped, outputsGrouped1)
                assertEquals(1, outputsGrouped?.colorOutputs?.size)
                assertNotNull(outputsGrouped?.depthOutput)
                assertEquals(2032, outputsGrouped?.colorOutputs?.get(0)?.width)
                assertEquals(1070, outputsGrouped?.colorOutputs?.get(0)?.height)
            }

            run {
                val outputsLeaf = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 715)
                }
                assertEquals(1, outputsLeaf?.colorOutputs?.size)
                assertNotNull(outputsLeaf?.depthOutput)
                assertEquals(2032, outputsLeaf?.colorOutputs?.get(0)?.width)
                assertEquals(1070, outputsLeaf?.colorOutputs?.get(0)?.height)
            }

            run {
                val outputsGrouped1 = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 697)
                }
                assertNotEquals(outputsGrouped1, outputsGrouped)
                assertEquals(1, outputsGrouped1?.colorOutputs?.size)
                assertNotNull(outputsGrouped1?.depthOutput)
                assertEquals(2032, outputsGrouped1?.colorOutputs?.get(0)?.width)
                assertEquals(1070, outputsGrouped1?.colorOutputs?.get(0)?.height)

                val outputsEndEvent = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 738)
                }
                assertEquals(outputsGrouped1, outputsEndEvent)
            }

            run {
                val outputsLeaf = withContext(rdDispatcher) {
                    capture.getTextureRGBBuffer.startSuspending(modelLifetime, 1244)
                }
                assertEquals(1, outputsLeaf?.colorOutputs?.size)
                assertNotNull(outputsLeaf?.depthOutput)
                assertEquals(2032, outputsLeaf?.colorOutputs?.get(0)?.width)
                assertEquals(1070, outputsLeaf?.colorOutputs?.get(0)?.height)
            }
        }

        suspend fun testRenderDocClient(lifetime: Lifetime, capture: RdcCapture) {
            val breakpoints = listOf(
                RdcSourceBreakpoint("Assets/Cube Shader.shader", 44u),
                RdcSourceBreakpoint("Assets/Cube Shader.shader", 62u),
                RdcSourceBreakpoint("Assets/Cube Shader.shader", 72u),
                RdcSourceBreakpoint("Assets/NewShader.shader", 44u),
                RdcSourceBreakpoint("Assets/NewShader.shader", 59u),
                RdcSourceBreakpoint("Assets/ShaderForSphere.shader", 20u),
                RdcSourceBreakpoint("Assets/mult.hlsl", 3u),
                RdcSourceBreakpoint("Assets/mult.hlsl", 7u),
                RdcSourceBreakpoint("Assets/Waves.shader", 58u)
            )

            assertDebugVertexStepByStepDisassembly(lifetime, capture)
            assertDebugVertexStepByStepShaderLab(lifetime, capture)
            assertTryDebugVertexStepByStep(lifetime, capture, breakpoints)
            assertTryDebugVertexWithBreakpoints(lifetime, capture, breakpoints)

            assertDebugPixelStepByStepDisassembly(lifetime, capture)
            assertDebugPixelStepByStepShaderLab(lifetime, capture)
            assertTryDebugPixelStepByStep(lifetime, capture, breakpoints)
            assertTryDebugPixelWithBreakpoints(lifetime, capture, breakpoints)

            assertVerticesTable(lifetime, capture)
            assertTexturesOutputs(lifetime, capture)
        }
    }
}
