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
            +"Hull"
            +"Domain"
            +"Geometry"
            +"Pixel"
            +"Compute"
            +"Task"
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

        val rdcDescriptorType = enum("rdcDescriptorType") {
            +"Unknown"
            +"ConstantBuffer"
            +"Sampler"
            +"ImageSampler"
            +"Image"
            +"Buffer"
            +"TypedBuffer"
            +"ReadWriteImage"
            +"ReadWriteTypedBuffer"
            +"ReadWriteBuffer"
            +"AccelerationStructure"
        }

        val rdcDescriptorAccess = structdef("rdcDescriptorAccess") {
            field("type", rdcDescriptorType)
            field("index", uint16)
            field("arrayElement", uint32)
        }

        val rdcUsedDescriptor = structdef("rdcUsedDescriptor") {
            field("access", rdcDescriptorAccess)
            field("resourceId", uint64.nullable)
        }

        val rdcStageInfo = structdef("rdcStageInfo") {
            field("currentVariables", array(rdcSourceVariableMapping))
            field("variableChanges", array(rdcShaderVariableChange))
            field("switchedDrawCall", bool)
        }

        val rdcShaderSampler = structdef("rdcShaderSampler") {
            field("name", string)
            field("fixedBindNumber", uint32)
            field("fixedBindSetOrSpace", uint32)
            field("bindArraySize", uint32)
        }

        val rdcShaderConstantType = structdef("rdcShaderConstantType") {
            field("name", string)
            field("flags", rdcVariableFlags)
            field("pointerTypeID", uint32)
            field("elements", uint32)
            field("arrayByteStride", uint32)
            field("baseType", rdcVarType)
            field("rows", uint8)
            field("columns", uint8)
            field("matrixByteStride", uint8)
        }

        val rdcShaderConstant = structdef("rdcShaderConstant") {
            field("name", string)
            field("byteOffset", uint32)
            field("bitFieldOffset", uint16)
            field("bitFieldSize", uint16)
            field("defaultValue", uint64)
            field("type", rdcShaderConstantType)
            field("typeMembers", array(this))
        }

        val rdcConstantBlock = structdef("rdcConstantBlock") {
            field("name", string)
            field("variables", array(rdcShaderConstant))
            field("fixedBindNumber", uint32)
            field("fixedBindSetOrSpace", uint32)
            field("bindArraySize", uint32)
            field("byteSize", uint32)
            field("bufferBacked", bool)
            field("inlineDataBytes", bool)
            field("compileConstants", bool)
        }

        val rdcTextureType = enum("rdcTextureType") {
            +"Unknown"
            +"Buffer"
            +"Texture1D"
            +"Texture1DArray"
            +"Texture2D"
            +"TextureRect"
            +"Texture2DArray"
            +"Texture2DMS"
            +"Texture2DMSArray"
            +"Texture3D"
            +"TextureCube"
            +"TextureCubeArray"
            +"Count"
        }

        val rdcShaderResource = structdef("rdcShaderResource") {
            field("textureType", rdcTextureType)
            field("descriptorType", rdcDescriptorType)
            field("name", string)
            field("variableType", rdcShaderConstantType)
            field("typeMembers", array(rdcShaderConstant))
            field("fixedBindNumber", uint32)
            field("fixedBindSetOrSpace", uint32)
            field("bindArraySize", uint32)
            field("isTexture", bool)
            field("hasSampler", bool)
            field("isInputAttachment", bool)
            field("isReadOnly", bool)
        }

        val rdcShaderReflection = structdef("rdcShaderReflection") {
            field("constantBlocks", array(rdcConstantBlock))
            field("samplers", array(rdcShaderSampler))
            field("readOnlyResources", array(rdcShaderResource))
            field("readWriteResources", array(rdcShaderResource))
        }

        val rdcResourceInfo = structdef("rdcResourceInfo") {
            field("id", uint64)
            field("name", string)
        }

        val rdcDrawCallDebugSession = classdef("rdcDrawCallDebugSession") {
            field("eventId", uint32)
            field("debugTrace", rdcDebugTrace)
            field("sourceFiles", array(rdcSourceFile))

            field("allResources", array(rdcResourceInfo))
            field("readOnlyResources", array(rdcUsedDescriptor))
            field("readWriteResources", array(rdcUsedDescriptor))
            field("samplers", array(rdcUsedDescriptor))
            field("shaderDetails", rdcShaderReflection)
        }

        val rdcDebugSession = classdef("rdcDebugSession") {
            property("drawCallSession", rdcDrawCallDebugSession)
            property("stageInfo", rdcStageInfo.nullable)
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

        val rdcSourceBreakpoint = structdef("rdcSourceBreakpoint") {
            field("sourceFilePath", string)
            field("line", uint)
        }

        val rdcCapture = classdef("rdcCapture") {
            field("api", rdcGraphicsApi)
            field("rootActions", array(rdcAction))

            callback("debugVertex", uint, rdcDebugSession)
            callback("debugPixel", uint, rdcDebugSession)
            callback("tryDebugVertex", immutableList(rdcSourceBreakpoint), rdcDebugSession)
            callback("tryDebugPixel", immutableList(rdcSourceBreakpoint), rdcDebugSession)
        }

        val rdcCaptureFile = classdef("rdcCaptureFile") {
            field("isLocalReplaySupported", bool)
            field("driverName", string)

            callback("openCapture", void, rdcCapture)
        }

        callback("openCaptureFile", string, rdcCaptureFile)
    }
}
