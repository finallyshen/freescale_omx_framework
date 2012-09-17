#!/bin/bash
if [ $# -ne 1 ]; then
	echo "Usage: ./repackage.sh RELEASE_PKG_NAME"
	echo "eg: ./repackage.sh imx-android-r9.3-rc1-codec"
	exit 127
fi

RELEASE_PKG_NAME=$1
OLD=imx-android-r9.4-rc4-codec

tar -zxvf $OLD-excluded.tar.gz
tar -zxvf $OLD-special.tar.gz

cd $OLD-excluded
tar -zxvf fsl_ac3_dec.tar.gz
cd ..
cd $OLD-special
tar -zxvf fsl_ms_codec.tar.gz
cd ..

cd $RELEASE_PKG_NAME-excluded
tar -zxvf fsl_ac3_dec.tar.gz
tar -zxvf fsl_ms_codec.tar.gz
cd ..

cp $RELEASE_PKG_NAME-excluded/fsl_ac3_dec/*.so $OLD-excluded/fsl_ac3_dec/
cp $RELEASE_PKG_NAME-excluded/fsl_ac3_dec/readme.txt $OLD-excluded/fsl_ac3_dec/
cp $RELEASE_PKG_NAME-excluded/fsl_ms_codec/*.so $OLD-special/fsl_ms_codec/
cp $RELEASE_PKG_NAME-excluded/fsl_ms_codec/readme.txt $OLD-special/fsl_ms_codec/
cp $RELEASE_PKG_NAME-excluded/vpu_fw_imx5x.tar.gz $OLD-excluded/

cp $RELEASE_PKG_NAME-excluded/i_MX_Android_Codec_Release_Notes.html $OLD-excluded/
cp $RELEASE_PKG_NAME-excluded/i_MX_Android_Codec_Release_Notes.html $OLD-special/

cd $OLD-excluded
tar -zcvf fsl_ac3_dec.tar.gz fsl_ac3_dec
rm -rf fsl_ac3_dec
cd ..
cd $OLD-special
tar -zcvf fsl_ms_codec.tar.gz fsl_ms_codec
rm -rf fsl_ms_codec
cd ..

mv $RELEASE_PKG_NAME-excluded $RELEASE_PKG_NAME-bak
mv $OLD-excluded $RELEASE_PKG_NAME-excluded
tar -zcvf $RELEASE_PKG_NAME-excluded.tar.gz $RELEASE_PKG_NAME-excluded
rm -rf $RELEASE_PKG_NAME-excluded
mv $RELEASE_PKG_NAME-bak $RELEASE_PKG_NAME-excluded
mv $OLD-special $RELEASE_PKG_NAME-special
tar -zcvf $RELEASE_PKG_NAME-special.tar.gz $RELEASE_PKG_NAME-special
rm -rf $RELEASE_PKG_NAME-special


echo Rlease package is done.
