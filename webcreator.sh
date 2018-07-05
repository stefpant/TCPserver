#!/usr/bin/env bash

if ! [[ $3 =~ ^[0-9]+$ ]] ; then
	echo "# Error: Given argument for sites is not integer!Exiting...";
	exit;
fi 
if (( $3 == 0 )); then
	echo "# Error: we need w>0 !Exiting...";
	exit;
fi

if ! [[ $4 =~ ^[0-9]+$ ]] ; then
	echo "# Error: Given argument for pages is not integer!Exiting...";
	exit;
fi
if (( $4 == 0 )); then
	echo "# Error: we need p>0 !Exiting...";
	exit;
fi


rootdir=$1;
text=$2;
sites=$(($3 - 1));
pages=$(($4 - 1));

if [ ! -d "$rootdir" ]; then
	echo "# Error: Root directory not found!Exiting...";
	exit;
fi
if [ ! -f "$text" ]; then
	echo "# Error: Input file not found!Exiting...";
	exit;
fi

textlen=$(wc -l < $text);
if ! [[ $textlen > 10000 ]]; then
	echo "# Error: Input file must have more than 10000 lines!Exiting...";
	exit;
fi

if [ ! -z "$(ls -A $rootdir)" ]; then
	echo "# Warning: directory is full, purging ...";
	rm -rf $rootdir/*;
fi
#rootdir now is empty

declare -A nnpages;
counter=0;
counterforuniques=0;
for i in $(seq 0 $sites); do #initialize sites and pages :-)
	mkdir $rootdir/site_$i;
	for j in $(seq 0 $pages); do #create page names
		while [ 1 ]; do
			name="$rootdir/site_$i/page${i}_$(( ( RANDOM % 10000 ) + 1 )).html";
			#touch "$1/site_$i/page_$(( ( RANDOM % 10000 ) + 1 )).html";
			if [ ! -d $name ]; then
				touch $name;
				chmod 777 $name;
				Anames[$counter]=$name;
				array[$counter]=$name;
				nnpages[$i,$j]=$name;
				let counter=counter+1;
				break;
			fi
		done
	done
done

counter=0;
for i in $(seq 0 $sites); do
	echo "#  Creating web site $i ..."
	for j in $(seq 0 $pages); do
		k=$(( RANDOM % ( $textlen - 2002 ) + 2 )); #step1
		m=$(( RANDOM % 999 + 1001 )); #step 2
		f=$(( $4/2 + 1 )); #step 3
		q=$(( $3/2 + 1 )); #step 4
		if (( $3 == 1 )); then
			q=0; #if w=1 no ex-links 
		fi
		if (( $3 == 2 && $4 == 1)); then
			q=1; #else q would be 2 but there isn't other ex-link
		fi

		currentfile=${array[$counter]};
		echo "#    Creating page $currentfile with $m lines starting at line $k ..."
		reps=$(( $f + $q ));
		Anames=( $(for i in ${Anames[@]}; do echo $i; done | shuf ));
		shufcounter=-1;
		echo "<!DOCTYPE html>" >> $currentfile;
		echo "<html>" >> $currentfile;
		echo "	<body>" >> $currentfile;
		for r in $(seq 0 $(( $reps-1 ))); do
			if (( $r == $f )); then
				shufcounter=-1;
			fi
			take=$(( $m / ($reps - $r) ));
			sed -n "$k,$(( $k+$take ))p" < $text >> $currentfile; # | sed -e "s/$/<br>/" to add new lines
			k=$(( $k + $take + 1)); #last time k should be k_0 + m_0
			m=$((  $m - $take )); #and m should be 0
			while [ 1 ]; do
				let shufcounter=shufcounter+1;
				linkname=${Anames[$shufcounter]};
				if (( $r < $f )); then #in-links
					if [[ $linkname =~ $rootdir/site_$i/page_$i_* ]]; then
						if [[ $linkname == $currentfile ]]; then
							if (( $pages > 1 )); then
								continue; #don't insert myself if not necessary
							fi
						fi
						echo "#     Adding link to $linkname";
						linkname=$( echo "$linkname" | sed "s/${rootdir}//" );
					 	echo "<a href=$linkname>$linkname</a>" >> $currentfile;
						notuniquesA[$counterforuniques]=$linkname;
						let counterforuniques=counterforuniques+1;
						break;
					fi #else try again
				else #out-links
					if [[ ! $linkname =~ $rootdir/site_$i/page_$i_* ]]; then
						echo "#     Adding link to $linkname";
						linkname=$( echo "$linkname" | sed "s/${rootdir}//" );
						echo "<a href=$linkname>$linkname</a>" >> $currentfile;
						notuniquesA[$counterforuniques]=$linkname;
						let counterforuniques=counterforuniques+1;
						break;
					fi
				fi
			done
			echo "<br>" >> $currentfile;
		done
		echo "	</body>" >> $currentfile;
		echo "</html>" >> $currentfile;
		let counter=counter+1;

	done
done

#check if all pages can be reached
uniquepr=$( for i in ${notuniquesA[@]}; do echo $i; done | sort -u | wc -w );
allpages=$(( $3 * $4 ));
if (( $allpages == $uniquepr ));then
	echo "# All pages have at least one incoming link"; 
fi

echo "# Done"
