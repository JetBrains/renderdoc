package com.jetbrains.renderdoc.rdClient

import com.jetbrains.rd.framework.*
import com.jetbrains.rd.util.PublicApi
import com.jetbrains.rd.util.lifetime.Lifetime
import com.jetbrains.rd.util.reactive.IPropertyView
import com.jetbrains.rd.util.reactive.IScheduler
import com.jetbrains.renderdoc.rdClient.model.RenderDocModel
import com.jetbrains.renderdoc.rdClient.model.renderDocModel

@PublicApi
class RenderDocClient(lifetime: Lifetime, @PublicApi val scheduler: IScheduler, sessionId: Long, port: Int) {
    companion object {
        suspend fun createWithHost(lifetime: Lifetime, scheduler: IScheduler, sessionId: Long, binDir: String): RenderDocClient {
            val hostLifetimeDef = lifetime.createNested()
            val host = RenderDocHost(hostLifetimeDef, binDir)

            val clientLifetimeDef = hostLifetimeDef.createNested()
            return RenderDocClient(clientLifetimeDef, scheduler, sessionId, host.getPort())
        }
    }

    @PublicApi
    val connected: IPropertyView<Boolean>
    @PublicApi
    val model: RenderDocModel

    init {
        val protocolId = "RenderDocClient::$sessionId"
        val client = SocketWire.Client(lifetime, scheduler, port, protocolId)
        val protocol = Protocol(protocolId, Serializers(), Identities(IdKind.Client), scheduler, client, lifetime)
        RenderDocModel.register(protocol.serializers)

        connected = client.connected
        model = protocol.renderDocModel
    }
}