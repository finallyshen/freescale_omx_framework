#!/bin/bash
if [ $# -ne 1 ]; then
	echo "Usage: ./android_rel.sh release_version"
	echo "eg: ./android_rel.sh r9.1-rc1"
        echo "Make sure you have built the libraries"
	exit 127
fi

VERSION=$1
PKG_NAME=imx-android-$VERSION-codec-excluded
BUILD_PATH=../../../out/target/product/imx51_bbg/system/lib
REL_PATH=../OpenMAXIL/release/lib/android

echo Building source code...
cd ../../../
source build/envsetup.sh
lunch
cd external/fsl_imx_omx
find . -name "*.h" | xargs -I [ touch [
mm > build.log 2>&1 || result=failed

if [ "xfailed" == "x$result" ]
then
    echo build failed, please see build.log
    exit 1
fi

rm build.log

#./prebuilt.sh

cd release_scripts/
if [ ! -d $PKG_NAME ]; then
    mkdir $PKG_NAME
fi

cp ../OpenMAXIL/doc/i_MX_Android_Codec_Release_Notes.html $PKG_NAME

rm $REL_PATH/fsl_ac3_dec/*.so
cp $BUILD_PATH/lib_omx_ac3_dec_v2_arm11_elinux.so $REL_PATH/fsl_ac3_dec
cp $BUILD_PATH/lib_ac3_dec_v2_arm11_elinux.so $REL_PATH/fsl_ac3_dec
cp $REL_PATH/fsl_ac3_dec -r $PKG_NAME
cd $PKG_NAME
find fsl_ac3_dec -name "*svn*" | xargs -I [ rm -fr [
tar -zcvf fsl_ac3_dec.tar.gz fsl_ac3_dec
rm -fr fsl_ac3_dec
cd ..

rm $REL_PATH/fsl_ra_dec/*.so
cp $BUILD_PATH/lib_omx_ra_dec_v2_arm11_elinux.so $REL_PATH/fsl_ra_dec
cp $BUILD_PATH/lib_realaudio_dec_v2_arm11_elinux.so $REL_PATH/fsl_ra_dec
cp $REL_PATH/fsl_ra_dec -r $PKG_NAME
cd $PKG_NAME
find fsl_ra_dec -name "*svn*" | xargs -I [ rm -fr [
tar -zcvf fsl_ra_dec.tar.gz fsl_ra_dec
rm -fr fsl_ra_dec
cd ..

rm $REL_PATH/fsl_rmvb_parser/*.so
cp $BUILD_PATH/lib_rm_parser_arm11_elinux.3.0.so $REL_PATH/fsl_rmvb_parser
cp $REL_PATH/fsl_rmvb_parser -r $PKG_NAME
cd $PKG_NAME
find fsl_rmvb_parser -name "*svn*" | xargs -I [ rm -fr [
tar -zcvf fsl_rmvb_parser.tar.gz fsl_rmvb_parser
rm -fr fsl_rmvb_parser
cd ..

cp $REL_PATH/vpu_fw_imx5x -r $PKG_NAME
cd $PKG_NAME
find vpu_fw_imx5x -name "*svn*" | xargs -I [ rm -fr [
tar -zcvf vpu_fw_imx5x.tar.gz vpu_fw_imx5x
rm -fr vpu_fw_imx5x
cd ..

rm $REL_PATH/fsl_ms_codec/*.so
cp $BUILD_PATH/lib_omx_wma_dec_v2_arm11_elinux.so $REL_PATH/fsl_ms_codec
cp $BUILD_PATH/lib_omx_wmv_dec_v2_arm11_elinux.so $REL_PATH/fsl_ms_codec
cp $BUILD_PATH/lib_asf_parser_arm11_elinux.3.0.so $REL_PATH/fsl_ms_codec
cp $BUILD_PATH/lib_wma10_dec_v2_arm12_elinux.so $REL_PATH/fsl_ms_codec
cp $BUILD_PATH/lib_WMV789_dec_v2_arm11_elinux.so $REL_PATH/fsl_ms_codec
cp $REL_PATH/fsl_ms_codec -r $PKG_NAME
cd $PKG_NAME
find fsl_ms_codec -name "*svn*" | xargs -I [ rm -fr [
tar -zcvf fsl_ms_codec.tar.gz fsl_ms_codec
rm -fr fsl_ms_codec
cd ..

echo Rlease package is done.
