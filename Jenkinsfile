pipeline {
    agent none
    options {
                buildDiscarder(logRotator(numToKeepStr: '25', daysToKeepStr: '7'))
            }


    parameters {
        choice(choices: 'Default\nRelease\nDebug',
            description: 'Default is decided by the in tree build scripts (see tools/buildsteps/defaultenv).',
            name: 'Configuration')
        booleanParam(name: 'RUN_TESTS',
                     defaultValue: true,
                     description: 'Turn this on if you want to build and run the xbmc unit tests based on gtest.')
        booleanParam(name: 'BUILD_BINARY_ADDONS',
                     defaultValue: true,
                     description: 'Weather we should be building the binary addons.')
        booleanParam(name: 'UPLOAD_RESULT',
                     defaultValue: true,
                     description: 'Whether the resulting builds should be uploaded to test-builds')
    }

    stages {
        stage('Build and Test') {
            environment {
                BUILD_DISPLAY_NAME = "${env.BRANCH_NAME[0..6]}-${env.BUILD_NUMBER}"
            }
            steps {
                parallel Windows32: {
                    node('win32') {
                        checkout scm
                        bat '%WORKSPACE%\\tools\\buildsteps\\windows\\win32\\prepare-env.bat'
                        bat '%WORKSPACE%\\tools\\buildsteps\\windows\\win32\\download-dependencies.bat'
                        bat '%WORKSPACE%\\tools\\buildsteps\\windows\\win32\\download-msys2.bat'
                        ansiColor('xterm') {
                            bat '%WORKSPACE%\\tools\\buildsteps\\windows\\win32\\make-mingwlibs.bat noprompt noclean sh'
                        }
                        bat '''IF "''' + params.BUILD_BINARY_ADDONS + '''" == "true" (
                              %WORKSPACE%\\tools\\buildsteps\\windows\\win32\\make-addons.bat
                            )'''
                        bat '%WORKSPACE%\\tools\\buildsteps\\windows\\win32\\BuildSetup.bat nobinaryaddons noclean noprompt sh'
                        bat '''IF "''' + params.RUN_TESTS + '''" == "true" (
                              %WORKSPACE%\\tools\\buildsteps\\windows\\win32\\run-tests.bat
                            )'''
                        junit allowEmptyResults: true, testResults: 'gtestresults.xml'
                        archiveArtifacts allowEmptyArchive: true, artifacts: 'gtestresults.xml'
                        load 'checkBinaryAddons.groovy'
                        echo 'SSH Publish'
                    }
                },
                Windows64: {
                    node('win64') {
                        checkout scm
                        bat '%WORKSPACE%\\tools\\buildsteps\\windows\\x64\\prepare-env.bat'
                        bat '%WORKSPACE%\\tools\\buildsteps\\windows\\x64\\download-dependencies.bat'
                        bat '%WORKSPACE%\\tools\\buildsteps\\windows\\x64\\download-msys2.bat'
                        ansiColor('xterm') {
                            bat '%WORKSPACE%\\tools\\buildsteps\\windows\\x64\\make-mingwlibs.bat noprompt noclean sh'
                        }
                        bat '''IF "''' + params.BUILD_BINARY_ADDONS + '''" == "true" (
                              %WORKSPACE%\\tools\\buildsteps\\windows\\x64\\make-addons.bat
                            )'''
                        bat '%WORKSPACE%\\tools\\buildsteps\\windows\\x64\\BuildSetup.bat nobinaryaddons noclean noprompt sh'
                        bat '''IF "''' + params.RUN_TESTS + '''" == "true" (
                              %WORKSPACE%\\tools\\buildsteps\\windows\\x64\\run-tests.bat
                            )'''
                        junit allowEmptyResults: true, testResults: 'gtestresults.xml'
                        archiveArtifacts allowEmptyArchive: true, artifacts: 'gtestresults.xml'
                        load 'checkBinaryAddons.groovy'
                        echo 'SSH Publish'
                    }
                },
                AndroidArm: {
                    node('android') {
                        ansiColor('xterm') {
                            checkout scm
                            sh '. $WORKSPACE/tools/buildsteps/android/prepare-depends'
                            sh '. $WORKSPACE/tools/buildsteps/android/configure-depends'
                            sh '. $WORKSPACE/tools/buildsteps/android/make-depends'
                            // will evaluate ${BUILD_BINARY_ADDONS} internally
                            sh '. $WORKSPACE/tools/buildsteps/android/prepare-xbmc'
                            sh '''if [ "''' + params.RUN_TESTS + '''" == "true" ]
                            then
                            export CONFIG_EXTRA=" --enable-gtest"
                            fi
                            . $WORKSPACE/tools/buildsteps/android/configure-xbmc'''
                            sh '. $WORKSPACE/tools/buildsteps/android/make-xbmc'
                            sh '''export RUN_SIGNSTEP="$WORKSPACE/../../android-dev/signing/doreleasesign.sh"
                                . $WORKSPACE/tools/buildsteps/android/package'''
                            junit allowEmptyResults: true, testResults: 'gtestresults.xml'
                            archiveArtifacts allowEmptyArchive: true, artifacts: 'gtestresults.xml'
                            load 'checkBinaryAddons.groovy'
                            echo 'SSH Publish'
                        }
                    }
                },
                AndroidArm64: {
                    node('android64') {
                        ansiColor('xterm') {
                            checkout scm
                            sh '. $WORKSPACE/tools/buildsteps/android-arm64-v8a/prepare-depends'
                            sh '. $WORKSPACE/tools/buildsteps/android-arm64-v8a/configure-depends'
                            sh '. $WORKSPACE/tools/buildsteps/android-arm64-v8a/make-depends'
                            // will evaluate ${BUILD_BINARY_ADDONS} internally
                            sh '. $WORKSPACE/tools/buildsteps/android-arm64-v8a/prepare-xbmc'
                            sh '''if [ "''' + params.RUN_TESTS + '''" == "true" ]
                            then
                            export CONFIG_EXTRA=" --enable-gtest"
                            fi
                            . $WORKSPACE/tools/buildsteps/android-arm64-v8a/configure-xbmc'''
                            sh '. $WORKSPACE/tools/buildsteps/android-arm64-v8a/make-xbmc'
                            sh '''export RUN_SIGNSTEP="$WORKSPACE/../../android-dev/signing/doreleasesign.sh"
                                . $WORKSPACE/tools/buildsteps/android-arm64-v8a/package'''
                            junit allowEmptyResults: true, testResults: 'gtestresults.xml'
                            archiveArtifacts allowEmptyArchive: true, artifacts: 'gtestresults.xml'
                            load 'checkBinaryAddons.groovy'
                            echo 'SSH Publish'
                        }
                    }
                },
                IOS: {
                    node('xcode6') {
                        ansiColor('xterm') {
                            checkout scm
                            sh '. $WORKSPACE/tools/buildsteps/ios/prepare-depends'
                            sh '. $WORKSPACE/tools/buildsteps/ios/configure-depends'
                            sh '. $WORKSPACE/tools/buildsteps/ios/make-depends'
                            sh '. $WORKSPACE/tools/buildsteps/ios/prepare-xbmc'
                            sh '. $WORKSPACE/tools/buildsteps/ios/configure-xbmc'
                            sh '. $WORKSPACE/tools/buildsteps/ios/make-xbmc'
                            sh '. $WORKSPACE/tools/buildsteps/ios/package'
                            junit allowEmptyResults: true, testResults: 'gtestresults.xml'
                            archiveArtifacts allowEmptyArchive: true, artifacts: 'gtestresults.xml'
                            load 'checkBinaryAddons.groovy'
                            echo 'SSH Publish'
                        }
                    }
                },
                IOSArm64: {
                    node('xcode6') {
                        ansiColor('xterm') {
                            checkout scm
                            sh '. $WORKSPACE/tools/buildsteps/ios/prepare-depends'
                            sh '. $WORKSPACE/tools/buildsteps/ios/configure-depends'
                            sh '. $WORKSPACE/tools/buildsteps/ios/make-depends'
                            sh '. $WORKSPACE/tools/buildsteps/ios/prepare-xbmc'
                            sh '. $WORKSPACE/tools/buildsteps/ios/configure-xbmc'
                            sh '. $WORKSPACE/tools/buildsteps/ios/make-xbmc'
                            sh '. $WORKSPACE/tools/buildsteps/ios/package'
                            junit allowEmptyResults: true, testResults: 'gtestresults.xml'
                            archiveArtifacts allowEmptyArchive: true, artifacts: 'gtestresults.xml'
                            load 'checkBinaryAddons.groovy'
                            echo 'SSH Publish'
                        }
                    }
                },
                OSX64: {
                    node('xcode6') {
                        ansiColor('xterm') {
                            checkout scm
                            sh '. $WORKSPACE/tools/buildsteps/osx64/prepare-depends'
                            sh '. $WORKSPACE/tools/buildsteps/osx64/configure-depends'
                            sh '. $WORKSPACE/tools/buildsteps/osx64/make-depends'
                            // will evaluate ${BUILD_BINARY_ADDONS} internally
                            sh '. $WORKSPACE/tools/buildsteps/osx64/prepare-xbmc'
                            sh '''if [ "''' + params.RUN_TESTS + '''" == "true" ]
                                then
                                export CONFIG_EXTRA=" --enable-gtest"
                                fi
                                . $WORKSPACE/tools/buildsteps/osx64/configure-xbmc'''
                            sh '. $WORKSPACE/tools/buildsteps/osx64/make-xbmc'
                            sh '. $WORKSPACE/tools/buildsteps/osx64/package'
                            sh '''if [ "${RUN_TESTS}" == "true" ]
                                then
                                . $WORKSPACE/tools/buildsteps/osx64/run-tests
                                fi'''
                            junit allowEmptyResults: true, testResults: 'gtestresults.xml'
                            archiveArtifacts allowEmptyArchive: true, artifacts: 'gtestresults.xml'
                            load 'checkBinaryAddons.groovy'
                            echo 'SSH Publish'
                        }
                    }
                },
                Linux64: {
                    node('linux') {
                        ansiColor('xterm') {
                            checkout scm
                            sh '. $WORKSPACE/tools/buildsteps/linux64/prepare-depends'
                            sh '. $WORKSPACE/tools/buildsteps/linux64/configure-depends'
                            sh '. $WORKSPACE/tools/buildsteps/linux64/make-depends'
                            // will evaluate ${BUILD_BINARY_ADDONS} internally
                            sh '. $WORKSPACE/tools/buildsteps/linux64/prepare-xbmc'
                            sh '''if [ "''' + params.RUN_TESTS + '''" == "true" ]
                            then
                            export CONFIG_EXTRA=" --enable-gtest"
                            fi
                            . $WORKSPACE/tools/buildsteps/linux64/configure-xbmc'''
                            sh '. $WORKSPACE/tools/buildsteps/linux64/make-xbmc'
                            sh '''if [ "''' + params.RUN_TESTS + '''" == "true" ]
                                then
                                . $WORKSPACE/tools/buildsteps/linux64/run-tests
                                fi'''
                            junit allowEmptyResults: true, testResults: 'gtestresults.xml'
                            archiveArtifacts allowEmptyArchive: true, artifacts: 'gtestresults.xml'
                            load 'checkBinaryAddons.groovy'
                            echo 'SSH Publish'
                        }
                    }
                },
                LinuxRBPI: {
                        node('rbpi') {
                            ansiColor('xterm') {
                                checkout scm
                                sh '. $WORKSPACE/tools/buildsteps/rbpi/prepare-depends'
                                sh '. $WORKSPACE/tools/buildsteps/rbpi/configure-depends'
                                sh '. $WORKSPACE/tools/buildsteps/rbpi/make-depends'
                                // will evaluate ${BUILD_BINARY_ADDONS} internally
                                sh '. $WORKSPACE/tools/buildsteps/rbpi/prepare-xbmc'
                                sh '. $WORKSPACE/tools/buildsteps/rbpi/configure-xbmc'
                                sh '. $WORKSPACE/tools/buildsteps/rbpi/make-xbmc'
                                sh '. $WORKSPACE/tools/buildsteps/rbpi/package'
                                junit allowEmptyResults: true, testResults: 'gtestresults.xml'
                                archiveArtifacts allowEmptyArchive: true, artifacts: 'gtestresults.xml'
                                load 'checkBinaryAddons.groovy'
                                echo 'SSH Publish'
                            }
                        }
                }
            }
        }
    }
}
