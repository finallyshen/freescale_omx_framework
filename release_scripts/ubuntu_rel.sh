#!/bin/bash
if [ $# -ne 1 ]; then
	echo "Usage: ./release.sh RELEASE_PKG_NAME"
	echo "eg: ./ubuntu_rel.sh omx_release_2_0_5"
	exit 127
fi

RELEASE=$1
PKG_NAME=omx_release
CODEC_PATH=../fsl_mad_multimedia_codec
PARSER_PATH=../fsl_mad_multimedia_layer
OMX_PATH=../OpenMAXIL/release

echo Building source code...
cd ..
make clean > /dev/null
make > build.log  2>&1 || result=failed
if [ "xfailed" == "x$result" ]
then
	echo build failed, please see build.log
	exit 1
fi
rm build.log
cd -

mkdir $PKG_NAME
cp ../Makefile.defines $PKG_NAME

mkdir $PKG_NAME/OSAL
mkdir $PKG_NAME/OSAL/ghdr
cp ../OSAL/ghdr/*.h $PKG_NAME/OSAL/ghdr

mkdir $PKG_NAME/utils
cp ../utils/Mem.h $PKG_NAME/utils/Mem.h
cp ../utils/Queue.h $PKG_NAME/utils/Queue.h

mkdir $PKG_NAME/OpenMAXIL
mkdir $PKG_NAME/OpenMAXIL/ghdr
cp ../OpenMAXIL/ghdr/*.h $PKG_NAME/OpenMAXIL/ghdr
mkdir $PKG_NAME/OpenMAXIL/test
cp ../OpenMAXIL/test/Makefile $PKG_NAME/OpenMAXIL/test
mkdir $PKG_NAME/OpenMAXIL/test/mp3_player
cp ../OpenMAXIL/test/mp3_player/mp3_player.c $PKG_NAME/OpenMAXIL/test/mp3_player
cp ../OpenMAXIL/test/mp3_player/mp3_player.h $PKG_NAME/OpenMAXIL/test/mp3_player
cp ../OpenMAXIL/test/mp3_player/Makefile $PKG_NAME/OpenMAXIL/test/mp3_player
mkdir $PKG_NAME/OpenMAXIL/test/video_player
cp ../OpenMAXIL/test/video_player/video_decoder_test.c $PKG_NAME/OpenMAXIL/test/video_player
cp ../OpenMAXIL/test/video_player/video_decoder_test.h $PKG_NAME/OpenMAXIL/test/video_player
cp ../OpenMAXIL/test/video_player/Makefile $PKG_NAME/OpenMAXIL/test/video_player
mkdir $PKG_NAME/OpenMAXIL/release
mkdir $PKG_NAME/OpenMAXIL/release/bin
mkdir $PKG_NAME/OpenMAXIL/release/registry
cp $OMX_PATH/registry/component_register $PKG_NAME/OpenMAXIL/release/registry
cp $OMX_PATH/registry/contentpipe_register $PKG_NAME/OpenMAXIL/release/registry
cp $OMX_PATH/registry/core_register $PKG_NAME/OpenMAXIL/release/registry
mkdir $PKG_NAME/OpenMAXIL/release/lib
cp $OMX_PATH/lib/*.so $PKG_NAME/OpenMAXIL/release/lib
cp $CODEC_PATH/release/lib/lib_vpu_wrapper.so $PKG_NAME/OpenMAXIL/release/lib
cp $OMX_PATH/asound.conf $PKG_NAME/OpenMAXIL/release
cd $PKG_NAME/OpenMAXIL/test
make > build.log  2>&1 || result=failed
if [ "xfailed" == "x$result" ]
then
	echo build failed, please see build.log
	exit 1
fi
make clean > /dev/null
rm build.log
cd -

echo Preparing release package...
cp $OMX_PATH/bin/mp3_player_arm11_elinux $PKG_NAME/OpenMAXIL/release/bin
cp $OMX_PATH/bin/video_decoder_test_arm11_elinux $PKG_NAME/OpenMAXIL/release/bin

chmod 755 $PKG_NAME/OpenMAXIL/release/bin/mp3_player_arm11_elinux
chmod 755 $PKG_NAME/OpenMAXIL/release/bin/video_decoder_test_arm11_elinux

mkdir omx_gm
mv $OMX_PATH/bin/omxgraph_manager_arm11_elinux omx_gm
tar -zcvf omx_gm_2_0_5.tar.gz omx_gm
rm -rf omx_gm

mkdir core_lib
cp $CODEC_PATH/release/lib/*.so core_lib
cp $PARSER_PATH/release/lib/*.so core_lib

tar -zcvf core_lib.tar.gz core_lib
rm -fr core_lib

rm $PKG_NAME/OpenMAXIL/release/lib/*ra_dec*.so
rm $PKG_NAME/OpenMAXIL/release/lib/*fake_video_render*.so
rm $PKG_NAME/OpenMAXIL/release/lib/*audio_fake_render*.so
rm $PKG_NAME/OpenMAXIL/release/lib/*video_fileread*.so
rm $PKG_NAME/OpenMAXIL/release/lib/*shared_fd_pipe*.so
rm $PKG_NAME/OpenMAXIL/release/lib/*template*.so

mkdir omx_debian
mkdir omx_debian/DEBIAN
cp control omx_debian/DEBIAN
chmod 755 -R omx_debian/DEBIAN
mkdir omx_debian/usr
mkdir omx_debian/usr/bin
mkdir omx_debian/usr/lib
mkdir omx_debian/etc
mkdir omx_debian/etc/omx_registry
cp $PKG_NAME/OpenMAXIL/release/lib/*.so omx_debian/usr/lib
cp $PKG_NAME/OpenMAXIL/release/bin/* omx_debian/usr/bin
cp $PKG_NAME/OpenMAXIL/release/asound.conf omx_debian/etc
cp $PKG_NAME/OpenMAXIL/release/registry/* omx_debian/etc/omx_registry/
rm omx_debian/etc/asound.conf

sudo dpkg -b omx_debian fsl-omx_2.0-5_armel.deb

mkdir $PKG_NAME/docs
mkdir $PKG_NAME/docs/html
cp ../OpenMAXIL/doc/Multimedia_Openmax_Ubuntu_Release_Notes.html $PKG_NAME/docs
cp ../OpenMAXIL/doc/EULA.txt $PKG_NAME/docs
#cp ../OpenMAXIL/doc/How_to_run_GraphManager.txt $PKG_NAME/docs
cp ../OpenMAXIL/doc/html/Freescale_Logo.jpg $PKG_NAME/docs/html

tar -zcvf $RELEASE.tar.gz $PKG_NAME
rm -fr $PKG_NAME
rm -fr omx_debian

mkdir release_pkg
mv $RELEASE.tar.gz release_pkg
mv omx_gm_2_0_5.tar.gz release_pkg
mv core_lib.tar.gz release_pkg
mv fsl-omx_2.0-5_armel.deb release_pkg
dpkg -x release_pkg/fsl-omx_2.0-5_armel.deb release_pkg/fsl-omx_2.0-5_debian_src
cd release_pkg
cd ..

echo Rlease package is done.
