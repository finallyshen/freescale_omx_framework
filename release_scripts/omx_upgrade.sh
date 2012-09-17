#!/bin/bash -x

set +x

re_checkout_all=0
build_omx=1
make_overlay_patch=0

# ================  default value for input =========================
TAG_FROM="imx-android-r9.1-rc3"
ANDROID_TO="up2date"
OMX_TO="up2date"
MYANDROID_DIR="../../../"    # default location of this script is myAndroid/external/fsl_imx_omx/release_scripts
COMMIT_FILE=""
VPU="v3"
PREBUILD_TO="up2date"

# ==================== global variables =============================
current_dir=`pwd`
target_dir=""   # place result tar.gz file here
repo_name=""
root=""    # another name for MYANDROID_DIR

# ==================== function definitions =========================


checkResult()
{
    if [ $? != 0 ]; then
        echo Fail!!!
        exit 0
    fi
}

log()
{
    printf "\n$1\n\n"
}

cleanUpGit()
{
    # clean up modified files
    if [ "`git status | grep modified`" != "" ]; then
        git checkout `git status | grep modified | awk '{printf("%s\n",$3);}' `
    fi

    # clean up new added files in include/media
    if [ "`git status | grep "include/media"`" != "" ]; then
        rm `git status | grep "include/media" | awk '{printf("%s ",$2);}'`
    fi

}

# get comments of commit and output to $1
GetLastCommit()
{
    file_name="-- $2"

    first_commit=0
    git log $file_name | while read -r line
    do
        if [ "${line:0:6}" = "commit" ]; then
            if [ $first_commit -eq 1 ]; then
                 break
            else
                 first_commit=1
            fi

        fi
        echo "        $line" >> $1
    done
}


showHelp()
{
    echo "Usage : omx_upgrade.sh parameters"
    echo "parameters:"
    echo "          -from [tag_from] "
    echo "          -android_to [android_to]   - optional. target of frameworks/base. default: up2date"
    echo "          -omx_to [omx_to]           - optional. target of external/fsl_imx_omx. default: up2date"
    echo "          -dir [path of myAndroid]"
    echo "          -patch [commit_file]       - optional. "
    echo "          -vpu [version]             - optional. version is "v2" or "v3", default is "v3" "
    echo "          -prebuild_to [prebuild_to] - optional. default: up2date. commit or tag for device/fsl-proprietary/omx/fsl-omx.mk"
    echo ""
    echo "Example:  ./omx_upgrade.sh -from imx-android-r9.2-rc3 -android_to imx-android-r9.4-rc3 -dir ../../../"
    echo ""
    echo "Notes:"
    echo "          [android_to] and [omx_to] can be \"up2date\", tag number, or commit number"
    echo "          [commit_file] contains the commit numbers of omx that need to build into this package"
    echo "                        commit numbers shall be in sequence from old to new"

}


# =================== get parameters ================================
set +x

if [ "$1" = "-h" ] || [ "$1" = "--help" ] ; then
    showHelp
    exit 0
fi

get_from=0
get_android_to=0
get_omx_to=0
get_dir=0
get_patch=0
get_vpu=0
get_prebuild=0

clearFlags()
{
    get_from=0
    get_android_to=0
    get_omx_to=0
    get_dir=0
    get_patch=0
    get_vpu=0
    get_prebuild=0
}

for arg in $@
do
    if [ "$arg" = "-from" ]; then
        clearFlags
        get_from=1
        continue
    fi
    if [ "$arg" = "-android_to" ]; then
        clearFlags
        get_android_to=1
        continue
    fi
    if [ "$arg" = "-omx_to" ]; then
        clearFlags
        get_omx_to=1
        continue
    fi
    if [ "$arg" = "-dir" ]; then
        clearFlags
        get_dir=1
        continue
    fi
    if [ "$arg" = "-patch" ]; then
        clearFlags
        get_patch=1
        continue
    fi
    if [ "$arg" = "-vpu" ]; then
        clearFlags
        get_vpu=1
        continue
    fi
    if [ "$arg" = "-prebuild_to" ]; then
        clearFlags
        get_prebuild=1
        continue
    fi

    if [ $get_from -eq 1 ]; then
        TAG_FROM=$arg
        clearFlags
        continue
    fi

    if [ $get_android_to -eq 1 ]; then
        ANDROID_TO=$arg
        clearFlags
        continue
    fi

    if [ $get_omx_to -eq 1 ]; then
        OMX_TO=$arg
        clearFlags
        continue
    fi

    if [ $get_dir -eq 1 ]; then
        MYANDROID_DIR=$arg
        clearFlags
        continue
    fi

    if [ $get_patch -eq 1 ]; then
        COMMIT_FILE=$arg
        clearFlags
        continue
    fi
    if [ $get_vpu -eq 1 ]; then
        VPU=$arg
        clearFlags
        continue
    fi
    if [ $get_prebuild -eq 1 ]; then
        PREBUILD_TO=$arg
        clearFlags
        continue
    fi
done

repo_name="repomad1/maddev_gingerbread"

echo $TAG_FROM | grep "r9."
if [ $? = 0 ]; then
    repo_name="repomad1/maddev_froyo"
fi

echo $TAG_FROM | grep "r9.2."
if [ $? = 0 ]; then
    repo_name="repomad1/maddev-imx-android-r9.2-postrelease"
fi

echo $TAG_FROM | grep "r9.4."
if [ $? = 0 ]; then
    repo_name="repomad1/maddev-imx-android-r9.4-postrelease"
fi

echo repo_name=$repo_name

if [ "$COMMIT_FILE" != "" ]; then
    if [ ! -f $COMMIT_FILE ]; then
        echo $COMMIT_FILE does not exist!!!
        exit 1
    fi
    COMMIT_FILE=$current_dir/$COMMIT_FILE
fi

if [ ! -d $MYANDROID_DIR ]; then
    echo $MYANDROID_DIR does not exist!!!
    exit 1
fi

cd $MYANDROID_DIR
root=$(pwd)
#echo root=$root
                    
if [ "$COMMIT_FILE" != "" ] && [ "$OMX_TO" = "up2date" ]; then
    echo "COMMIT_FILE conflict with up2date"
    exit 1
fi

echo check input correctness!!!
echo ==================================================
echo "tag_from      =>  $TAG_FROM"
echo "android_to    =>  $ANDROID_TO"
echo "omx_to        =>  $OMX_TO"
echo "myAndroid_dir =>  $root"
echo "commit_file   =>  $COMMIT_FILE"
echo "prebuild_to   =>  $PREBUILD_TO"
echo ==================================================

sleep 5

cd $root ;                                                                 checkResult

set +x
source build/envsetup.sh ;                                                  checkResult
echo "please choose 53 smd eng ...................."
lunch ;                                                                   checkResult
set -x

set +x
log " =============== checkout for $TAG_FROM ========================"
set -x

cd $root/frameworks/base
cleanUpGit

cd $root
if [ -f repo ]; then 
	repo_cmd="./repo"
else
	which repo
	if [ $? -eq 0 ]; then
	    repo_cmd="repo"
	else
	    echo "No repo command found!!!"
	    exit 1
	fi
fi

cd $root/frameworks/base
git tag | grep "$TAG_FROM"
if [ $? -ne 0 ]; then
	cd $root
	./repo forall -c git fetch repomad1 --tags > log 2>&1;       # may fail
fi

if [ "$re_checkout_all" -eq 1 ]; then
	cd $root
	$repo_cmd forall -c git checkout $TAG_FROM > log 2>&1;         # may fail
fi

cd $root/frameworks/base
git checkout -f $TAG_FROM > log 2>&1 ;                                   checkResult

 
set +x
log "====================== make hardware/libhardware package ============="
set -x

to=0
from=0

cd $root
if [ $make_overlay_patch = 1 ] && [ -d hardware/libhardware ]; then
	cd hardware/libhardware

	git checkout -f $ANDROID_TO ;						checkResult
	git log | egrep -nHI 'da2ce520d52bea04bacc88cfd6c0eaba1254dbf3'  
	if [ $? -eq 0 ]; then
		to=1
	fi

	git checkout -f $TAG_FROM;						checkResult
	git log | egrep -nHI 'da2ce520d52bea04bacc88cfd6c0eaba1254dbf3'  
	if [ $? -eq 0 ]; then
		from=1
	fi

	if [ $from -eq 0 ] && [ $to -eq 1 ]; then
		if [ -f hardware_libhardware.patch ];then
			rm hardware_libhardware.patch ;             		checkResult
		fi
		git format-patch -1 da2ce520d52bea04bacc88cfd6c0eaba1254dbf3 ;  checkResult
		mv 0001-ENGR00133115-overlay-support-tv-out-and-dual-display.patch hardware_libhardware.patch; checkResult
		git apply hardware_libhardware.patch ;				checkResult
	fi

fi
set +x
log "================ make patch for frameworks/base ================"
set -x

if [ "$TAG_FROM" = "$ANDROID_TO" ]; then
        make_frameworks_base_patch=false
else
	make_frameworks_base_patch=true
fi

if [ "$make_frameworks_base_patch" = "true" ];then

	echo search date of last commit in git log of $TAG_FROM 

	cd $root/frameworks/base ;                                             checkResult

	git log > git_log

	# // get date of last commit in TAG_FROM //

	date_from=0

	set +x

	while read -r line 
	do
	    if [[ "${line:0:6}" = "commit" ]]; then
		    read -r line 
		    read -r line 
		    if [[ ${line:0:4} = "Date" ]]; then
			date_from=${line#*"Date: "}
			echo date_from is $date_from
			break
		    fi
	    fi
	done < git_log

	set -x

	if [[ "$date_from" = 0 ]]; then
	    echo read date_from fail in $TAG_FROM frameworks/base!!!  
	    exit 0
	fi

	rm -f git_log


	if [ $ANDROID_TO != "up2date" ]; then
	    git checkout -f $ANDROID_TO > log 2>&1;   # may fail
	else
	    git remote update;                            checkResult
	    git checkout -f $repo_name;                   checkResult
	fi


	git log > git_log

	commit_from=0

	set +x

	while read -r line
	do
	    # // search last commit of $tag_to in git log //
	    if [[ "${line:0:6}" = "commit" ]]; then
		commit_to=${line#*"commit "}
		read -r line
		read -r line
		if [[ "${line:0:4}" = "Date" ]]; then
		    date_to=${line#*"Date: "}
		    echo date_to is $date_to,commit_to is $commit_to
		    break
		else
		    echo read date in git log of $tag_to in frameworks/base fail!!!
		    exit 0
		fi
	    else
		echo read git log of $tag_to in frameworks/base fail!!!
		exit 0
	    fi
	done < git_log

	while read -r line
	do
	    # // search last commit of $TAG_FROM in git log of $tag_to //
	    if [[ "${line:0:6}" = "commit" ]]; then
		commit_temp=${line#*"commit "}
		read -r line
		read -r line
		if [[ "${line:0:4}" = "Date" ]]; then
		    date_temp=${line#*"Date: "}
		    #echo date_temp is "$date_temp" date_from is "$date_from"
		    if [[ "$date_temp" = "$date_from" ]]; then
			commit_from=$commit_temp
			echo commit_from=$commit_from
			break
		    fi
		fi

	    fi
	done < git_log

        # order of last commits in 10.2.1 is different with in main branch
        if [[ $TAG_FROM = "imx-android-r10.2.1" ]]; then
            commit_from=e6bec697b08b7e5fa44f01cf5bcca180262c7a40
        fi

	set -x

	if [[ $commit_from = 0 ]]; then
	    echo search commit_from of date: $date_from in git log fail!!!
	    exit 0
	fi

        # // generate patch files and output to all.patch //

	rm -f git_log

    if [[ $commit_from = $commit_to ]];then
        make_frameworks_base_patch=false
    fi
	
fi

if [ "$make_frameworks_base_patch" = "true" ];then

	rm -f *.patch
	git format-patch $commit_from..$commit_to ;                                  checkResult

        # // output valid patch name into team_patch_files //
	egrep -l 'lijian|Song Bing|Xu Mao|yongbing|Kevinbing' *.patch > team_patch_files            # $? is 1 when not found

	# // only record commit log when we have patchs //
    rm -f commit_log
    GetLastCommit commit_log

fi

set +x
log " =================== check to TAG_FROM ========================="
set -x

if [ "$make_frameworks_base_patch" = "true" ];then
	cd $root/frameworks/base
	git checkout -f $TAG_FROM > log 2>&1
fi

set +x
log " ========== apply patch to TAG_FROM of frameworks/base =============="
set -x

if [ "$make_frameworks_base_patch" = "true" ];then

	cd $root/frameworks/base

	cleanUpGit

	rm -f frameworks_base.patch
	touch frameworks_base.patch

	if [ -s team_patch_files ]; then
		for line in `cat team_patch_files`
		do
			git apply $line
			if [ $? -eq 0 ]; then
				cat $line >> frameworks_base.patch
			else
				set +x
				echo ""
				echo git apply $line in frameworks/base failed!!!
				echo press [Enter] to continue, or [Ctrl]+[c] to exit:
				read 
				set -x
			fi
		done

	fi

	rm -f team_patch_files
	rm -f 0*.patch

	git status | egrep -nHI 'libs/camera'
	if [ $? -eq 0 ]; then
		cd libs/camera ;						checkResult
		touch *.h *.cpp ;						checkResult
		mm > log 2>&1 ;							checkResult
		cd $root/frameworks/base
	fi

	git status | egrep -nHI 'services/camera/libcameraservice'
	if [ $? -eq 0 ]; then
		cd $root
		if [ -d hardware/mx5x/libcamera ]; then
			cd hardware/mx5x/libcamera;				checkResult
			touch *.h *.cpp
			mm > log 2>&1 ;						checkResult
		fi
		if [ -d hardware/imx/mx5x/libcamera ]; then
			cd hardware/imx/mx5x/libcamera;				checkResult
			touch *.h *.cpp
			mm > log 2>&1 ;						checkResult
		fi
		cd $root/frameworks/base/services/camera/libcameraservice ;	checkResult
		touch *.cpp ;							checkResult
		mm > log 2>&1 ;							checkResult
		cd $root/frameworks/base
	fi
fi

set +x
log "======================== make omx patch ========================"
set -x

cd $root/external
if [ ! -d fsl_imx_omx ]; then
    echo "fsl_imx_omx does not exist!!!"
    exit 0
    #git clone git://repomad1/platform/external/fsl_imx_omx.git ;                checkResult
    #cd fsl_imx_omx ;                                                           checkResult
fi

patch_count=0

if [ "$COMMIT_FILE" != "" ]; then
    cd $root/external/fsl_imx_omx ;                                            checkResult

    cleanUpGit

    git checkout -f repomad1/maddev ;                                            checkResult

    git checkout -b upgrade_temp_branch_$RANDOM repomad1/maddev;                        checkResult


    while read -r line
    do
        if [ "$line" != "" ]; then
            git format-patch -1 $line
            if [ $? -eq 0 ]; then
                patch_name=`git format-patch -1 $line`
            else
                echo "make patch for $line fail !!!"
                continue
            fi
            patch_count=$[$patch_count + 1]
            mv $patch_name $patch_count.patch
        fi
    done < $COMMIT_FILE
fi


set +x
log "========================= get omx source code =================="
set -x


cd $root/external/fsl_imx_omx ;                                            checkResult

cleanUpGit

if [ "$OMX_TO" = "up2date" ]; then
    git remote update;                                                  checkResult
    git checkout -f repomad1/maddev ;                                      checkResult
    git log > git_log                
    
    while read -r line
    do
        if [ "${line:0:6}" = "commit" ]; then
            omx_last_commit=`echo "$line" | awk '{printf("%s\n",$2);}'`    
	    break
        fi
    done < git_log
    rm -f git_log
else
    git checkout $OMX_TO ;                                              checkResult
    if [ $patch_count -ne 0 ]; then
        index=1
        while [ $index -le $patch_count ]
        do
            git apply $index.patch
            rm -f $index.patch
            index=$[$index + 1]
        done
    fi
fi

set +x
log "======================= modify BoardConfig ====================="
set -x

cd $root/device/fsl/imx5x

rm -f temp

set +x

while read -r line
do
    echo $line | sed -e 's/EXCLUDED_CODEC_V2_BUILD := false/EXCLUDED_CODEC_V2_BUILD := true/g' >> temp
done < BoardConfigCommon.mk

set -x

mv temp BoardConfigCommon.mk ;                                                  checkResult


set +x
log " ============================= build omx ======================="
set -x

if [ $build_omx = 1 ]; then

    cd $root/external/fsl_imx_omx ;                                                       checkResult 

    find . -name "*.h" | xargs -I [ touch [ ;                                       checkResult

    mm > log 2>&1 ;                                                                 checkResult
fi

cd $root
if [ ! -d device/fsl-proprietary/omx ]; then
        mkdir device/fsl-proprietary/omx ;                                   checkResult
fi

cd device/fsl-proprietary/omx

if [ ! -d lib ]; then
        mkdir lib;                                                          checkResult
fi

if [ ! -d registry ]; then
        mkdir registry;                                                     checkResult
fi

## copy so files to device according to fsl-omx.mk ##

cd $root/device/fsl

cleanUpGit

if [ "$PREBUILD_TO" = "up2date" ]; then
    git remote update;                                                                             checkResult
    git checkout -f $repo_name;                                                                   checkResult
    git remote update;                                                                             checkResult
else
    git checkout $PREBUILD_TO ;                                                                 checkResult
fi

if [ "$PREBUILD_TO" = "up2date" ]; then
    git log > git_log                
    
    while read -r line
    do
        if [ "${line:0:6}" = "commit" ]; then
            prebuild_last_commit=`echo "$line" | awk '{printf("%s\n",$2);}'`    
	    break
        fi
    done < git_log
    rm -f git_log
fi


set +x

cd $root
rm -f device/fsl-proprietary/omx/lib/*

while read -r line
do
  findstring=`echo $line | egrep -I 'lib/lib'`
  if [ "$findstring" != "" ]; then
     #echo $findstring
     end=${findstring#*lib/}
     sofile=`echo $end | awk '{printf("%s ",$1);}'`
     #echo $sofile
     if [ -f out/target/product/imx53_smd/system/lib/$sofile ]; then
        cp out/target/product/imx53_smd/system/lib/$sofile device/fsl-proprietary/omx/lib
     fi
  fi
done < device/fsl-proprietary/omx/fsl-omx.mk

set -x

#cp external/fsl_imx_omx/OpenMAXIL/release/registry/*_register  device/fsl-proprietary/omx/registry

cp external/fsl_imx_omx/OpenMAXIL/release/registry/core_register  device/fsl-proprietary/omx/registry
cp external/fsl_imx_omx/OpenMAXIL/release/registry/contentpipe_register  device/fsl-proprietary/omx/registry

if [ "$VPU" = "v2" ]; then
    if [ ! -f external/fsl_imx_omx/OpenMAXIL/release/registry/component_register.v2 ]; then
	echo "external/fsl_imx_omx/OpenMAXIL/release/registry/component_register.v2 does not exist!!!"
	exit 1
    fi
    cp external/fsl_imx_omx/OpenMAXIL/release/registry/component_register.v2  device/fsl-proprietary/omx/registry/component_register
else
    cp external/fsl_imx_omx/OpenMAXIL/release/registry/component_register  device/fsl-proprietary/omx/registry/component_register
fi

#set +x
#log " ========== build frameworks/base =========="
#set -x

#cd $root_from ;                                                             checkResult
#cd frameworks/base ;                                                        checkResult
#mm > log 2>&1 ;                                                             checkResult


set +x
log "========================== make omx tgz ========================"
set -x

current_date=`date +%Y%m%d_%H%M`

if [ ${ANDROID_TO} = "up2date" ]; then
    android_to_name=$current_date
else
    android_to_name=${ANDROID_TO}
fi

if [ ${OMX_TO} = "up2date" ]; then
    omx_to_name=$current_date
else
    omx_to_name=${OMX_TO}
fi

# if it's a commit_number, just take 10 at beginning

if [ ${#android_to_name} -eq 40 ]; then
    android_to_name=${android_to_name:0:10}
fi

if [ ${#omx_to_name} -eq 40 ]; then
    omx_to_name=${omx_to_name:0:10}
fi

#to_name=${android_to_name}_${omx_to_name}
to_name=${omx_to_name}
final_name=${TAG_FROM}_${current_date}

cd $current_dir
target_dir=$current_dir/Upgrade_${final_name}

rm -rf $target_dir
mkdir $target_dir 

cd $root

tar czvf omx.tar.gz \
              frameworks/base/include/media/OMXMediaScanner.h  \
              frameworks/base/include/media/OMXMetadataRetriever.h \
              frameworks/base/include/media/OMXPlayer.h \
              device/fsl-proprietary/omx/lib \
              device/fsl-proprietary/omx/registry \
              device/fsl-proprietary/omx/fsl-omx.mk
checkResult

cd $root/device/fsl
cleanUpGit

set +x
log "===================== make excluded package ===================="
set -x

cd ${root}/external/fsl_imx_omx
echo $TAG_FROM | grep "r9."
if [ $? -eq 0 ]; then
	script="android_codec_package_r9.sh"
	if [ ! -f $script ]; then
		script="android_codec_package.sh"
	fi
else
	script="android_codec_package.sh"
fi
	
if [ ! -f $script ]; then
    git checkout repomad1/maddev -- $script ;                                      checkResult
    sh_is_copied=1
fi

./$script ${final_name} ../../out/target/product/imx53_smd/system/lib;            checkResult

if [ $sh_is_copied -eq 1 ]; then
    rm $script
fi


set +x
log "====================== collect packages ========================"
set -x


cd ${root}/external/fsl_imx_omx
mv imx-android-${final_name}-codec-excluded.tar.gz $target_dir;                 checkResult
mv imx-android-${final_name}-codec-special.tar.gz $target_dir;                 checkResult

mv $root/omx.tar.gz $target_dir
if [ -s $root/frameworks/base/frameworks_base.patch ]; then 
    mv $root/frameworks/base/frameworks_base.patch $target_dir;                  checkResult
fi

cd $target_dir

if [ "$COMMIT_FILE" != "" ]; then
    cp $COMMIT_FILE ./commit_file ;                                              checkResult
fi

if [ -f README.TXT ]; then
    rm README.txt
fi

if [ -f $root/hardware/libhardware/hardware_libhardware.patch ];then
	cp $root/hardware/libhardware/hardware_libhardware.patch . ;             checkResult
fi

readme_content="                         \n
1.                                       \n
If frameworks_base.patch exists          \n
$ cd \${YOUR_ANDROID_SRC_DIR}/frameworks/base \n
$ git apply frameworks_base.patch        \n
                                         \n
2.                                       \n
If hardware_libhardware.patch exists          \n
$ cd \${YOUR_ANDROID_SRC_DIR}/hardware/libhardware \n
$ git apply hardware_libhardware.patch        \n
                                         \n
3.                                       \n
$ cd \${YOUR_ANDROID_SRC_DIR}            \n
$ tar -xzvf omx.tar.gz                   \n
                                         \n
$ cd device/fsl-proprietary/omx          \n
$ touch lib/*                            \n
$ touch registry/*                       \n
                                         \n
4.                                       \n
$ cd \${YOUR_ANDROID_SRC_DIR}            \n
$ source ./build/envsetup.sh             \n
$ lunch imx51_bbg-eng                    \n
$ mm                                     \n
or                                       \n
$ cd \${YOUR_ANDROID_SRC_DIR}            \n
$  make PRODUCT-imx51_bbg-eng            \n
"

echo -e $readme_content > README.txt

cp $root/external/fsl_imx_omx/OpenMAXIL/doc/EULA.txt . ;                                    checkResult

file_list="README.txt omx.tar.gz EULA.txt "

if [ "$make_frameworks_base_patch" = "true" ] && [ -f frameworks_base.patch ] ; then
    file_list="$file_list frameworks_base.patch"
fi

if [ -f hardware_libhardware.patch ]; then
    file_list="$file_list hardware_libhardware.patch"
fi

tar czvf Upgrade_${final_name}.tar.gz $file_list;                               checkResult
rm -f $file_list

# ================= generate log.txt ================================

if [ -f log.txt ]; then
	rm -f log.txt
fi

echo -e "tag_from = $TAG_FROM \n"                               >> log.txt
echo "android_to = $ANDROID_TO"                                 >> log.txt
if [ -f ${root}/frameworks/base/commit_log ]; then
    echo "    last commit of frameworks/base:"                  >> log.txt
    cat ${root}/frameworks/base/commit_log                      >> log.txt
    rm -f ${root}/frameworks/base/commit_log
fi

echo "omx_to = $OMX_TO"                                         >> log.txt
echo "    last commit of external/fsl_imx_omx:"                 >> log.txt
cd ${root}/external/fsl_imx_omx ;                               checkResult
GetLastCommit ${target_dir}/log.txt
cd $target_dir

echo "prebuild_to = $PREBUILD_TO"                               >> log.txt
echo "    last commit of device/fsl: "                          >> log.txt
cd ${root}/device/fsl;                                          checkResult
GetLastCommit ${target_dir}/log.txt proprietary/omx/fsl-omx.mk
cd $target_dir

echo " "                                                        >> log.txt
echo "    last commit of external/linux-lib: "                  >> log.txt
cd ${root}/external/linux-lib;                                  checkResult
GetLastCommit ${target_dir}/log.txt 
cd $target_dir

set +x
echo ""
echo "upgrade packaging finished in `basename $target_dir`"
echo ""

exit 0

