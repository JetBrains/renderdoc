package com.jetbrains.renderdoc.rd.gen

import com.jetbrains.rd.generator.nova.*
import com.jetbrains.rd.generator.nova.PredefinedType.*


@Suppress("unused")
object RenderDocModel : Ext(RenderDocRoot) {
    init {
        val rdcGraphicsApi = enum("rdcGraphicsApi") {
            +"Unknown"
            +"D3D11"
            +"D3D12"
            +"OpenGL"
            +"Vulkan"
        }

        val rdcActionFlags = flags("rdcActionFlags") {
            +"MeshDispatch"
            +"Drawcall"
        }

        val rdcSourceFile = structdef("rdcSourceFile") {
            field("name", string)
            field("content", string)
        }

        val rdcDebugStack = structdef("rdcDebugStack") {
            field("stepIndex", int)
            field("sourceFileIndex", int)
            field("lineStart", uint)
            field("lineEnd", uint)
            field("columnStart", uint)
            field("columnEnd", uint)
        }

        val rdcLineBreakpoint = structdef("rdcLineBreakpoint") {
            field("sourceFileIndex", uint)
            field("line", uint)
        }

        val rdcDebugSession = classdef("rdcDebugSession") {
            field("sourceFiles", array(rdcSourceFile))
            property("currentStack", rdcDebugStack.nullable)

            sink("stepInto", void)
            sink("stepOver", void)
            sink("resume", void)
            sink("addBreakpoint", rdcLineBreakpoint)
            sink("removeBreakpoint", rdcLineBreakpoint)
        }

        val rdcAction = structdef("rdcAction") {
            field("eventId", uint)
            field("actionId", uint)
            field("name", string)
            field("flags", rdcActionFlags)
            field("children", array(this))
        }

        val rdcCapture = classdef("rdcCapture") {
            field("api", rdcGraphicsApi)
            field("rootActions", array(rdcAction))

            callback("debugVertex", uint, rdcDebugSession)
        }

        val rdcCaptureFile = classdef("rdcCaptureFile") {
            field("isLocalReplaySupported", bool)
            field("driverName", string)

            callback("openCapture", void, rdcCapture)
        }

        callback("openCaptureFile", string, rdcCaptureFile)
    }
}
