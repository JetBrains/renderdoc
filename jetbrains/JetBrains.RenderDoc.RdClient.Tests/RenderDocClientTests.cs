using JetBrains.Lifetimes;
using JetBrains.RenderDoc.RdClient;

namespace JetBrains.RenderDoc.Tests;

public class RenderDocClientTests
{
    [Test]
    public static void Test()
    {
        var sessionId = 12345L;
        var lifetimeDefinition = Lifetime.Define();
        var lifetime = lifetimeDefinition.Lifetime;
        var client = new RenderDocClient();
        var timeOutLifetime = lifetime.CreateTerminatedAfter(TimeSpan.FromSeconds(500));
        var startup = client.Start(timeOutLifetime, sessionId);
        startup.Wait(timeOutLifetime);
    }
}