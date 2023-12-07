using System.Diagnostics;
using System.Text.RegularExpressions;
using JetBrains.Diagnostics;
using JetBrains.Lifetimes;

namespace JetBrains.RenderDoc.RdClient;

internal class RenderDocHost
{
    // spdlog message in format [date-time] [group] [logLevel] msg
    private static readonly Regex HostMessageRegex = new(@"^\[[^]]+\] \[([^]]+)\] \[(trace|debug|info|warn|err|critical|off)\] (.*)", RegexOptions.Compiled);
    private static readonly Regex HostIntroductionRegex = new(@"^HOST_INTRODUCTION: PORT=(\d+)", RegexOptions.Compiled);

    private TaskCompletionSource<int>? myExitCodeSource;

    public Task<int> HostExitCode => myExitCodeSource?.Task ?? Task.FromException<int>(new InvalidOperationException("Host wasn't started"));
    
    /**
     * Starts host bounded to <paramref name="lifetime"/> and returns host port.
     * Host exit code may be awaited and obtained with <see cref="HostExitCode"/>.
     */
    public Task<int> Start(Lifetime lifetime) =>
        lifetime.Execute(() =>
        {
            var startInfo = new ProcessStartInfo
            {
                FileName = "runtimes/osx-arm64/RenderDocHost",
                UseShellExecute = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true
            };

            var process = new Process { StartInfo = startInfo };
            process.EnableRaisingEvents = true;
            process.StartInfo = startInfo;
            
            var exitCodeSource = new TaskCompletionSource<int>();
            var portCompletionSource = new TaskCompletionSource<int>();
            process.Exited += (src, _) =>
            {
                var p = (Process)src;
                myExitCodeSource?.SetResult(p.ExitCode);
                portCompletionSource.TrySetCanceled();
            };
            
            var introduced = false;
            var serverLog = Log.GetLog<RenderDocClient>().GetSublogger("Host");
            process.OutputDataReceived += (_, args) =>
            {
                if (args.Data is not { } message)
                    return;
                if (!introduced && HostIntroductionRegex.Match(message) is { Success: true } match)
                {
                    portCompletionSource.SetResult(int.Parse(match.Groups[1].Value));
                    introduced = true;
                }
                else if (serverLog.IsVersboseEnabled())
                    LogHostMessage(serverLog, LoggingLevel.VERBOSE, message);
            };
            process.ErrorDataReceived += (_, args) =>
            {
                if (args.Data is { } message)
                    LogHostMessage(serverLog, LoggingLevel.ERROR, message);
            };

            if (!process.Start())
                return Task.FromException<int>(new FileNotFoundException("Failed to start RenderDocHost process"));

            myExitCodeSource = exitCodeSource;
            
            process.BeginErrorReadLine();
            process.BeginOutputReadLine();
            
            lifetime.OnTermination(() => process.Kill());
            return portCompletionSource.Task;
        });

    private static void LogHostMessage(ILog serverLog, LoggingLevel defaultLoggingLevel, string message)
    {
        if (HostMessageRegex.Match(message) is { Success: true } match)
        {
            var loggingLevel = match.Groups[2].Value switch
            {
                "trace" => LoggingLevel.TRACE,
                "debug" => LoggingLevel.VERBOSE,
                "info" => LoggingLevel.INFO,
                "warn" => LoggingLevel.WARN,
                "err" => LoggingLevel.ERROR,
                "critical" => LoggingLevel.FATAL,
                _ => defaultLoggingLevel
            };
            serverLog.Log(loggingLevel, $"{match.Groups[1]} | {match.Groups[3]}");
        }
        else
            serverLog.Log(defaultLoggingLevel, message);
    }
}