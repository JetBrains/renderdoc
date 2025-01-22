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
                capture.debugVertex.startSuspending(sessionLifetime, 784u)
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
                capture.debugVertex.startSuspending(sessionLifetime, 732u)
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
                capture.tryDebugVertex.startSuspending(sessionLifetime, breakpoints)
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
                capture.tryDebugVertex.startSuspending(sessionLifetime, breakpoints)
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
        }
    }
}
