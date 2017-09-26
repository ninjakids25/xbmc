echo 'Checking binary addons'

def pathesFailure = 'cmake/addons/.failure'
def pathesSuccess = 'cmake/addons/.success'

echo 'Checkpoint 2'

if ( fileExists(pathesSuccess) ) {
    echo "GROOVY: binary addons succeeded marker exists!"
    summary = manager.createSummary("accept.png").appendText("<h1>The following binary addons were built successfully:</h1><ul>", false)
    fileInputStream = readFile(pathesSuccess)
    fileInputStream.eachLine { line ->
        summary.appendText("<li><b>" + line + "</b></li>", false)
    }
    summary.appendText("</ul>", false)
}

echo 'Checkpoint 3'

if ( fileExists(pathesFailure) ) {
    echo "GROOVY: binary addons failed marker exists!"
    manager.addWarningBadge("Build of binary addons failed.")
    summary = manager.createSummary("warning.gif").appendText("<h1>Build of binary addons failed. This is treated as non-fatal. Following addons failed to build:</h1><ul>", false, false, false, "red")
    fileInputStream = readFile(pathesFailure)
    fileInputStream.eachLine { line ->
        summary.appendText("<li><b>" + line + "</b></li>", false)
    }
    summary.appendText("</ul>", false)
    manager.buildUnstable()
}
