package com.jetbrains.renderdoc.rd.gen

import com.jetbrains.rd.generator.nova.*
import com.jetbrains.rd.generator.nova.PredefinedType.*
import com.jetbrains.rd.generator.nova.PredefinedType.Companion.int16
import com.jetbrains.rd.generator.nova.PredefinedType.Companion.int32
import com.jetbrains.rd.generator.nova.PredefinedType.Companion.int64
import com.jetbrains.rd.generator.nova.PredefinedType.Companion.int8
import com.jetbrains.rd.generator.nova.PredefinedType.Companion.uint16
import com.jetbrains.rd.generator.nova.PredefinedType.Companion.uint32
import com.jetbrains.rd.generator.nova.PredefinedType.Companion.uint64
import com.jetbrains.rd.generator.nova.PredefinedType.Companion.uint8
import com.jetbrains.rd.generator.nova.cpp.Cpp17Generator
import com.jetbrains.rd.generator.nova.Enum

fun Enum.appendEntry(name: String, value: Int) {
    (+name).setting(Cpp17Generator.EnumConstantValue, value)
}

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

        val rdcShaderStage = enum("rdcShaderStage") {
            +"Vertex"
//            appendEntry("First", 0)
            +"Hull"
//            appendEntry("Tess_Control", 1)
            +"Domain"
//            appendEntry("Tess_Eval", 2)
            +"Geometry"
            +"Pixel"
//            appendEntry("Fragment", 4)
            +"Compute"
            +"Task"
//            appendEntry("Amplification", 6)
            +"Mesh"
            +"RayGen"
            +"Intersection"
            +"AnyHit"
            +"ClosestHit"
            +"Miss"
            +"Callable"
            +"Count"
        }

        val rdcVarType = enum("rdcVarType") {
            appendEntry("Float", 0)
            +"Double"
            +"Half"
            +"SInt"
            +"UInt"
            +"SShort"
            +"UShort"
            +"SLong"
            +"ULong"
            +"SByte"
            +"UByte"
            +"Bool"
            +"Enum"
            +"Struct"
            +"GPUPointer"
            +"ConstantBlock"
            +"ReadOnlyResource"
            +"ReadWriteResource"
            +"Sampler"
            appendEntry("Unknown", 0xFF)
        }

        val rdcVariableFlags = enum("rdcVariableFlags") {
            +"NoFlags"
            appendEntry("RowMajorMatrix", 0x0001)
            appendEntry("HexDisplay", 0x0002)
            appendEntry("BinaryDisplay", 0x0004)
            appendEntry("RGBDisplay", 0x0008)
            appendEntry("R11G11B10", 0x0010)
            appendEntry("R10G10B10A2", 0x0020)
            appendEntry("UNorm", 0x0040)
            appendEntry("SNorm", 0x0080)
            appendEntry("Truncated", 0x0100)
        }

        val rdcDebugVariableType = enum("rdcDebugVariableType") {
            +"Undefined"
            +"Input"
            +"Constant"
            +"Sampler"
            +"ReadOnlyResource"
            +"ReadWriteResource"
            +"Variable"
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

        val rdcShaderValue = structdef("rdcShaderValue") {
            field("f32v", array(float))
            field("s32v", array(int32))
            field("u32v", array(uint32))
            field("f64v", array(double))
            field("f16v", array(uint16))
            field("u64v", array(uint64))
            field("s64v", array(int64))
            field("u16v", array(uint16))
            field("s16v", array(int16))
            field("u8v", array(uint8))
            field("s8v", array(int8))
        }

        val rdcShaderVariable = structdef("rdcShaderVariable") {
            field("name", string)
            field("rows", uint)
            field("columns", uint)
            field("type", rdcVarType)
            field("flags", rdcVariableFlags)
            field("value", rdcShaderValue)
            field("members", array(this))
        }

        val rdcDebugVariableReference = structdef("rdcDebugVariableReference") {
            field("name", string)
            field("type", rdcDebugVariableType)
            field("component", uint32)
        }

        val rdcShaderVariableChange = structdef("rdcShaderVariableChange") {
            field("before", rdcShaderVariable)
            field("after", rdcShaderVariable)
        }

        val rdcSourceVariableMapping = structdef("rdcSourceVariableMapping") {
            field("name", string)
            field("type", rdcVarType)
            field("rows", uint32)
            field("columns", uint32)
            field("offset", uint32)
            field("signatureIndex", int32)
            field("variables", array(rdcDebugVariableReference))
        }

        val rdcDebugTrace = structdef("rdcDebugTrace") {
            field("shaderStage", rdcShaderStage)
            field("inputs", array(rdcShaderVariable))
            field("constantBlocks", array(rdcShaderVariable))
            field("readOnlyResources", array(rdcShaderVariable))
            field("readWriteResources", array(rdcShaderVariable))
            field("samplers", array(rdcShaderVariable))
            field("sourceVars", array(rdcSourceVariableMapping))
        }

        val rdcStageVariableInfo = structdef("rdcStageVariableInfo") {
            field("currentVariables", array(rdcSourceVariableMapping))
            field("variableChanges", array(rdcShaderVariableChange))
        }

        val rdcDebugSession = classdef("rdcDebugSession") {
            field("debugTrace", rdcDebugTrace)
            field("sourceFiles", array(rdcSourceFile))
            property("currentStack", rdcDebugStack.nullable)

            sink("stepInto", void)
            sink("stepOver", void)
            sink("resume", void)
            sink("addBreakpoint", rdcLineBreakpoint)
            sink("removeBreakpoint", rdcLineBreakpoint)
            callback("getStageVariableInfo", void, rdcStageVariableInfo)
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
            callback("debugPixel", uint, rdcDebugSession)
        }

        val rdcCaptureFile = classdef("rdcCaptureFile") {
            field("isLocalReplaySupported", bool)
            field("driverName", string)

            callback("openCapture", void, rdcCapture)
        }

        callback("openCaptureFile", string, rdcCaptureFile)
    }
}
