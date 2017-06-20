/*
 * libsercomm - Jenkins build script
 *
 * Copyright (C) the libsercomm contributors. All rights reserved.
 *
 * This file is part of libsercomm, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
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
                cmake -H. -Bbuild_linux_default \
                  -DBUILD_SHARED_LIBS=ON \
                  -DWITH_GITINFO=ON \
                  -DWITH_EXAMPLES=ON
                rm -f build_linux_default/*.tar.gz
                cmake --build build_linux_default --target package
                """

            sh  """
                cmake -H. -Bbuild_linux_rpi2 \
                    -DCMAKE_TOOLCHAIN_FILE=/opt/toolchains/rpi2/Toolchain.cmake \
                    -DBUILD_SHARED_LIBS=ON \
                    -DWITH_GITINFO=ON \
                    -DWITH_EXAMPLES=ON
                rm -f build_linux_rpi2/*.tar.gz
                cmake --build build_linux_rpi2 --target package
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
                cmake -H. -Bbuild_macos_default \
                  -DBUILD_SHARED_LIBS=ON \
                  -DWITH_GITINFO=ON \
                  -DWITH_EXAMPLES=ON
                rm -f build_macos_default/*.tar.gz
                cmake --build build_macos_default --target package
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

            def archs = ['x86', 'x64']
            def archs_gen = ['', ' Win64']

            for (i = 0; i < archs.size(); i++) {
                bat """
                    cmake -H. -Bbuild_windows_${archs[i]} \
                        -G "Visual Studio 15 2017${archs_gen[i]}"
                        -DBUILD_SHARED_LIBS=ON \
                        -DWITH_GITINFO=ON \
                        -DWITH_EXAMPLES=ON
                    del /q /s build_windows_${archs[i]}\\*.zip
                    cmake --build build_windows_${archs[i]} --target package
                    """
            }

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
