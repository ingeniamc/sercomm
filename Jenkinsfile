/*
 * libsercomm - Jenkins build script
 */

def builds = [:]

builds['Linux'] = {
    node('linux && libs') {
        if (env.BRANCH_NAME == 'master') {
            deleteDir()
        }

        stage('Linux') {
            checkout scm

            sh  """
                cmake -H. -Bbuild \
                  -DBUILD_SHARED_LIBS=ON \
                  -DWITH_GITINFO=ON \
                  -DWITH_EXAMPLES=ON
                rm -f build/*.tar.gz
                cmake --build build --target package
                """

            step([$class: 'WarningsPublisher',
                 consoleParsers: [[parserName: 'GNU C Compiler 4 (gcc)']]])

            archiveArtifacts artifacts: 'build*/*.tar.gz'
        }
    }
}

builds['macOS'] = {
    node('macos && libs') {
        if (env.BRANCH_NAME == 'master') {
            deleteDir()
        }

        stage('macOS') {
            checkout scm

            sh  """
                cmake -H. -Bbuild \
                  -DBUILD_SHARED_LIBS=ON \
                  -DWITH_GITINFO=ON \
                  -DWITH_EXAMPLES=ON
                rm -f build/*.tar.gz
                cmake --build build --target package
                """

            step([$class: 'WarningsPublisher',
                 consoleParsers: [[parserName: 'Clang (LLVM based)']]])

            archiveArtifacts artifacts: 'build*/*.tar.gz'
        }
    }
}

builds['Windows'] = {
    node('windows && libs') {
        if (env.BRANCH_NAME == 'master') {
            deleteDir()
        }

        stage('Windows') {
            checkout scm

                bat """
                    cmake -H. -Bbuild \
                        -G "Visual Studio 14 2015"
                        -DBUILD_SHARED_LIBS=ON \
                        -DWITH_GITINFO=ON \
                        -DWITH_EXAMPLES=ON
                    del /q /s build\\*.zip
                    cmake --build build --target package
                    """

            step([$class: 'WarningsPublisher',
                 consoleParsers: [[parserName: 'MSBuild']]])

            archiveArtifacts artifacts: 'build*/*.zip'
        }
    }
}

if (env.BRANCH_NAME == 'master') {
    builds['Docs'] = {
        node('linux && libs') {
            deleteDir()

            stage('Docs') {
                checkout scm
                sh 'git submodule update --init'

                sh  """
                    cmake -H. -Bbuild -DWITH_DOCS=ON
                    mkdir -p build/docs
                    cmake --build build --target docs
                    tar -zcf docs-sercomm.tar.gz -C build/docs .
                    """

                archiveArtifacts artifacts: 'docs-sercomm.tar.gz'

                sh """ rm -rf /mnt/docs/libs/sercomm
                       cp -r build/docs /mnt/docs/libs/sercomm
                   """

                step([$class: 'WarningsPublisher',
                      consoleParsers: [[parserName: 'Doxygen']]])
            }
        }
    }
}

parallel builds
