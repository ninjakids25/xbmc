echo 'Checking binary addons'

def pathesFailure = [manager.build.workspace.toString(),'cmake','addons','.failure']
def pathesSuccess = [manager.build.workspace.toString(),'cmake','addons','.success']

def binary_addons_failed = new hudson.FilePath(manager.build.workspace.channel, pathesFailure.join(File.separator))
def binary_addons_succeeded = new hudson.FilePath(manager.build.workspace.channel, pathesSuccess.join(File.separator))

if ( binary_addons_succeeded != null && binary_addons_succeeded.exists() ) {
    manager.listener.logger.println "GROOVY: binary addons succeeded marker exists!"
    summary = manager.createSummary("accept.png")
    summary.appendText("<h1>The following binary addons were built successfully:</h1><ul>", false)
    fileInputStream = binary_addons_succeeded.read()
    fileInputStream.eachLine { line ->
        summary.appendText("<li><b>" + line + "</b></li>", false)
    }
    summary.appendText("</ul>", false)
}

if ( binary_addons_failed != null && binary_addons_failed.exists() ) {
    manager.listener.logger.println "GROOVY: binary addons failed marker exists!"
    manager.addWarningBadge("Build of binary addons failed.")
    summary = manager.createSummary("warning.gif")
    summary.appendText("<h1>Build of binary addons failed. This is treated as non-fatal. Following addons failed to build:</h1><ul>", false, false, false, "red")
    fileInputStream = binary_addons_failed.read()
    fileInputStream.eachLine { line ->
        summary.appendText("<li><b>" + line + "</b></li>", false)
    }
    summary.appendText("</ul>", false)
    manager.buildUnstable()
}
