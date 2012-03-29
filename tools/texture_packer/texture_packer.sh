#!/bin/sh

if [ $# -ne 1 ]
then
	echo "Usage: texture_packer atlas_to_create"
	exit 1
fi

output=/tmp/${1}.png
desc=/tmp/${1}.desc
tmp_image=/tmp/tmp___.png

rm ${desc}

# Take the output of texture_packer as input and then use image magick to compose the atlas
while read data; do
	if echo $data | grep -q Atlas
	then
		# create image
		w=`echo $data | cut -d: -f2 | cut -d, -f1`
		h=`echo $data | cut -d, -f2`
		echo "Atlas size: $w x $h"
		convert -size ${w}x${h} xc:transparent ${output}
	else
		image=`echo $data | cut -d, -f1`
		x=`echo $data | cut -d, -f2`
		y=`echo $data | cut -d, -f3`
		w=`echo $data | cut -d, -f4`
		h=`echo $data | cut -d, -f5`
		rot=`echo $data | cut -d, -f6`
		
		if [ ${rot} -ne "0" ]
		then
			convert -rotate 90 ${image} ${tmp_image}
		else
			cp ${image} ${tmp_image}
		fi

		echo "Adding ${image} at ${w}x${h}+${x}+${y} (rotation:${rot})"
		convert -geometry ${w}x${h}+${x}+${y} -composite $output $tmp_image $output
		image=`echo ${image} | sed 's/assets\///'`
		echo "${image},${x},${y},${w},${h},${rot}" >> ${desc}
	fi
done
