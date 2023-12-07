using JetBrains.Annotations;
using JetBrains.Collections.Viewable;
using JetBrains.Lifetimes;
using JetBrains.Rd;
using JetBrains.Rd.Impl;
using JetBrains.RenderDoc.RdClient.Model;

namespace JetBrains.RenderDoc.RdClient;

[PublicAPI]
public class RenderDocClient
{
    public async Task Start(Lifetime lifetime, long sessionId)
    {
        var protocolId = $"RenderDocClient::{sessionId}";

        var scheduler = DefaultScheduler.Instance;
        var taskScheduler = scheduler.AsTaskScheduler();
        var host = new RenderDocHost();
        var port = await host.Start(lifetime);
        
        var serializers = new Serializers(taskScheduler, null);
        RenderDocModel.RegisterDeclaredTypesSerializers(serializers);
        
        var client = new SocketWire.Client(lifetime, scheduler, port, protocolId);
        var protocol = new Protocol(protocolId, serializers, new Identities(IdKind.Client), scheduler, client, lifetime);
        await lifetime.Start(taskScheduler, () =>
        {
            var model = new RenderDocModel(lifetime, protocol);
            model.Pong.Advise(lifetime, _ =>
            {
                Console.WriteLine("Pong!!!");
            });
            model.Ping();
            model.Ping();
            model.Shutdown();
        });
        await host.HostExitCode;
    }
}