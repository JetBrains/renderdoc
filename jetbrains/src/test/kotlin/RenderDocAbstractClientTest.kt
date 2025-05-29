import com.jetbrains.rd.framework.createBackgroundScheduler
import com.jetbrains.rd.framework.protocolOrThrow
import com.jetbrains.rd.util.lifetime.Lifetime
import com.jetbrains.rd.util.lifetime.LifetimeDefinition
import com.jetbrains.rd.util.lifetime.waitTermination
import com.jetbrains.rd.util.threading.coroutines.adviseSuspend
import com.jetbrains.rd.util.threading.coroutines.asCoroutineDispatcher
import com.jetbrains.rd.util.threading.coroutines.createTerminatedAfter
import com.jetbrains.renderdoc.rdClient.RenderDocClient
import com.jetbrains.renderdoc.rdClient.model.RdcAction
import com.jetbrains.renderdoc.rdClient.model.RdcCapture
import com.jetbrains.renderdoc.rdClient.model.RdcCaptureFile
import com.jetbrains.renderdoc.rdClient.model.RdcDebugPixelInput
import com.jetbrains.renderdoc.rdClient.model.RdcDebugSession
import com.jetbrains.renderdoc.rdClient.model.RdcDebugStack
import com.jetbrains.renderdoc.rdClient.model.RdcDebugVertexInput
import com.jetbrains.renderdoc.rdClient.model.RdcSourceFilesInAction
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertNull
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.fail
import java.time.Duration
import kotlin.collections.component1
import kotlin.collections.component2
import kotlin.collections.iterator
import kotlin.coroutines.EmptyCoroutineContext
import kotlin.io.path.pathString
import kotlin.io.path.toPath

abstract class RenderDocAbstractClientTest {
    companion object {
        class FrameSessionTracker {
            val frames = mutableListOf<RdcDebugStack>()
            val drawCallChanges = mutableMapOf<Int, UInt>()
            val sourceNamesPerDrawCall = mutableListOf<List<String>?>()

            suspend fun init(sessionLifetime: LifetimeDefinition, rdDispatcher: CoroutineDispatcher, session: RdcDebugSession) {
                withContext(rdDispatcher) {
                    session.sessionState.adviseSuspend(sessionLifetime, rdDispatcher) { state ->
                        if (state != null) {
                            val stack = state.currentStack
                            if (stack.drawCallId != frames.lastOrNull()?.drawCallId) {
                                drawCallChanges[frames.size] = stack.drawCallId
                                sourceNamesPerDrawCall.add(state.drawCallSession?.sourceFiles?.map { it.name })
                            } else {
                                assertNull(state.drawCallSession)
                            }
                            frames.add(stack)
                        } else {
                            sessionLifetime.terminate()
                        }
                    }
                }
            }
        }

        fun assertSourceNamesPerDrawCall(frameTracker: FrameSessionTracker, expected: List<List<String>?>, eventsToSkip: List<UInt>) {
            assertEquals(frameTracker.drawCallChanges.size, frameTracker.sourceNamesPerDrawCall.size)
            frameTracker.drawCallChanges.onEachIndexed { i, change ->
                if (frameTracker.sourceNamesPerDrawCall[i] != null) {
                    assertTrue(expected[i] == frameTracker.sourceNamesPerDrawCall[i])
                } else {
                    assertTrue(eventsToSkip.contains(change.value))
                }
            }
        }

        fun getFileUsagesInEvent(replay: RdcCapture) : Map<UInt, RdcSourceFilesInAction> {
            val fileUsages = mutableMapOf<UInt, RdcSourceFilesInAction>()
            for (action in replay.rootActions) {
                for ((eventId, filesUsagesInfo) in getFileUsagesInEvent(action)) {
                    fileUsages[eventId] = filesUsagesInfo
                }
            }
            return fileUsages
        }

        private fun getFileUsagesInEvent(action: RdcAction) : Map<UInt, RdcSourceFilesInAction> {
            if (action.children.isEmpty()) {
                val fileUsages = action.usedSourceFilePaths ?: return emptyMap()
                return mapOf(action.eventId to fileUsages)
            }

            val fileUsages = mutableMapOf<UInt, RdcSourceFilesInAction>()
            for (child in action.children) {
                for ((eventId, filesUsagesInfo) in getFileUsagesInEvent(child)) {
                    fileUsages[eventId] = filesUsagesInfo
                }
            }
            return fileUsages
        }

        fun assertSourceFilesUsages(value: RdcSourceFilesInAction, expectedEntrypoints: Set<String>, expectedOthers: Set<String>) {
            assertEquals(expectedEntrypoints, value.entrypointPaths.toSet())
            assertEquals(expectedOthers, value.otherIncludedFilePaths.toSet())
        }

        suspend fun assertSessionFinishesImmediately(modelLifetime: Lifetime, capture: RdcCapture, input: Any, debugSingleCall: Boolean) {
            val rdDispatcher = capture.protocolOrThrow.scheduler.asCoroutineDispatcher
            val sessionLifetime = modelLifetime.createNested()
            val debugSession = withContext(rdDispatcher) {
                when (input) {
                    is RdcDebugVertexInput -> (if (debugSingleCall) capture.debugVertex else capture.tryDebugVertex).startSuspending(sessionLifetime, input)
                    is RdcDebugPixelInput -> (if (debugSingleCall) capture.debugPixel else capture.tryDebugPixel).startSuspending(sessionLifetime, input)
                    else -> Assertions.fail("Unexpected input type detected")
                }
            }

            val frameTracker = FrameSessionTracker().also { it.init(sessionLifetime, rdDispatcher, debugSession) }

            sessionLifetime.waitTermination()

            assertEquals(emptyList<RdcDebugStack>(), frameTracker.frames)
            assertEquals(hashMapOf<Int, UInt>(), frameTracker.drawCallChanges)
            assertEquals(emptyList<List<String>?>(), frameTracker.sourceNamesPerDrawCall)
        }
    }

    protected abstract val captureDirectoryPath: String
    protected abstract val driver: String

    fun run(sessionId: Long, fileName: String, testBody: suspend (Lifetime, RdcCaptureFile, RdcCapture) -> Unit) {
        val lifetime = Lifetime.Eternal.createTerminatedAfter(Duration.ofSeconds(120), EmptyCoroutineContext)

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

            val rdcSample = javaClass.classLoader.getResource("${captureDirectoryPath}/${fileName}")?.toURI()?.toPath()?.pathString ?: fail("Failed to load sample resource")

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
                testBody(modelLifetime, captureFile, capture)
            }
        }
    }
}