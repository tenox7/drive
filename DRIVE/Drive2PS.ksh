#!/bin/sh
#@(#) $Revision: 1.2 $
# DRIVE Scene to PostScript translator
# By Edgar Circenis 4/28/93

# (c) Copyright Hewlett-Packard Company 2001
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


function usage
{
	echo "Usage: Drive2PS [drive scene file...]"
	echo "		[-s]		# Print one scene per page"
	echo "		[-f]		# Print one file per page"
	echo "				# Default is to print multiple files per page"
	echo "		[-b]		# Show scene boundaries"
	echo "		[-g]		# Show 100ft grid"
	echo "		[-G]		# Don't draw the ground"
	echo "		[-xmin]		# Don't print scenes below xmin (def=-2000)"
	echo "		[-xmax]		# Don't print scenes above xmax (def=2000)"
	echo "		[-zmin]		# Don't print scenes below zmin (def=-2000)"
	echo "		[-zmax]		# Don't print scenes above zmax (def=2000)"
	echo "		[-l]		# Label with filename"
	echo "		[-L <string>]	# Label with supplied string (overrides -l)"
	echo "		[-c]		# Add coordinates to existing label"
	echo "		[-D <0,1,2>]	# Enable debugging"
	exit 1
}

debug=0
ss=0
sb=0
sg=0
sf=0
xmin=-2000
xmax=2000
zmin=-2000
zmax=2000
draw_ground=1
flabel=0
llabel=0
sc=0
LABEL=""

while :
do
	case $1 in
		-D) debug=$2 ;;
		-s) if [ $sf = 1 ]; then
			usage >&2
		    fi
		    ss=1 ;;
		-f) if [ $ss = 1 ]; then
			u >&2sage
		    fi
		    sf=1 ;;
		-b) sb=1 ;;
		-g) sg=1 ;;
		-G) draw_ground=0 ;;
		-xmin) xmin=$2;shift ;;
		-xmax) xmax=$2;shift ;;
		-zmin) zmin=$2;shift ;;
		-zmax) zmax=$2;shift ;;
		-l) flabel=1 ;;
		-L) llabel=1 ; LABEL=$2; shift ;;
		-c) sc=1 ;;
		-*) usage >&2 ;;
		*) break ;;
	esac
	shift
done

cat << "_EOF_" > /tmp/Drive2PS$$
function COS(a) { return cos(a*6.283185307/360.0) }
function SIN(a) { return sin(a*6.283185307/360.0) }
function tempinit() {
	tempstr=""
}
function tempprint(s) {
	tempstr=sprintf "%s%s\n",tempstr,s
}
function hprint(s) {
	if (length(sceneheader[sh])>2000) {
		sceneheader[++sh]=""
	}
	sceneheader[sh] = sprintf "%s%s\n",sceneheader[sh],s
}
function vtprint(s) {
	if (length(scenevtop[svt])>2000) {
		scenevtop[++svt]=""
	}
	scenevtop[svt] = sprintf "%s%s\n",scenevtop[svt],s
}
function tprint(s) {
	if (length(scenetop[st])>2000) {
		scenetop[++st]=""
	}
	scenetop[st] = sprintf "%s%s\n",scenetop[st],s
}
function bprint(s) {
	if (length(scenebottom[sb])>2000) {
		scenebottom[++sb]=""
	}
	scenebottom[sb] = sprintf "%s%s\n",scenebottom[sb],s
}
function MIN(a,b) { return (a<b)?a:b }
function MAX(a,b) { return (a>b)?a:b }
function randomhill() {
	hx=1400*rand-700
	hy=1400*rand-700
	bprint("gsave")
	bprint(hx " " hy " translate")
	bprint("0.5 0.5 0 setrgbcolor")
	#bprint("0.80 setgray")
	bprint("0 72 360 { gsave 0 0 moveto rotate 0 350 lineto rand 100 mod -50 add rand 150 mod 250 add lineto closepath fill grestore } for")
	bprint("grestore")
}
function debug() { if (DEBUG==1) tprint("% read:" $0) }
BEGIN {
	FS="[ 	:]+"
	pi=3.14159265
	dirty=0
	newpage=1
	xmax=-2000
	xmin=2000
	zmax=-2000
	zmin=2000
	sn=0
	object="none"
	sh=svt=st=sb=0
}
/^[ 	]/ {
	sub(/^[ 	]*/,"")
}
/^#/ {
	debug()
	next
}
$0~/^Object:/ || $0~/^Construct:/ || $0~/^Scene Location:/ {
	if (Yrot=="") Yrot=0
	if (object=="road" || object=="hillroad") {
		Length=COS(Xrot)*Length
		Width=COS(Zrot)*Width
		tprint("% " object)
		tprint("gsave")
		tprint(x " " z " translate")
		tprint(Yrot " rotate")
		tprint(Width " setlinewidth")
		tprint("newpath")
		tprint("0 0 moveto")
		tprint(0 " " Length " lineto")
		tprint("stroke")
		tprint("grestore")

	} else if (object=="spline" ) {
		tprint("% " object)
		tprint("gsave")
		tprint(x " " z " translate")
		tprint(Yrot " rotate")
		tprint(Width " setlinewidth")
		tprint("newpath")
		tprint(spline[1,1] " " spline[1,3] " moveto")	
		for (j=1;j<idx;j++) {
			midx=(spline[j,1]+spline[j+1,1])/2
			midz=(spline[j,3]+spline[j+1,3])/2
			tprint(midx " " midz " lineto")	
		}
		tprint(spline[idx,1] " " spline[idx,3] " lineto")	
		tprint("stroke")
		tprint("grestore")

	} else if (object=="curve" || object=="spiral") {
		tprint("% " object)
		tprint("gsave")
		tprint(Width " setlinewidth")
		tprint(x " " z " " Radius-Width/2 " " Yrot " " Yrot+Angle " arc")
		tprint("stroke")
		tprint("grestore")
	} else if (object=="ramp" || object=="twistramp") {
		tprint("% " object)
		tprint("gsave")
		tprint(x " " z " translate")
		tprint(Yrot " rotate")
		tprint(Width " setlinewidth")
		tprint("0 0 moveto")
		tprint(0 " " Length " lineto")
		tprint("stroke")
		tprint("grestore")

		tprint("% sides of ramp")
		tprint("gsave")
		tprint(x " " z " translate")
		tprint(Yrot " rotate")
		tprint("0 0.2 0 setrgbcolor")
		#tprint("0.25 setgray")
		tprint("newpath")
		tprint(-Width/2 " " 0 " moveto")
		tprint(-Width/2-10 " " Length " lineto")
		tprint(-Width/2 " " Length " lineto")
		tprint("closepath")
		tprint("fill")
		tprint("newpath")
		tprint(Width/2 " " 0 " moveto")
		tprint(Width/2+10 " " Length " lineto")
		tprint(Width/2 " " Length " lineto")
		tprint("closepath")
		tprint("fill")
		tprint("grestore")

	} else if (object=="bridge") {
		tprint("% bridge")
		tprint("gsave")
		tprint(x " " z " translate")
		tprint(Yrot " rotate")
		tprint(Width " setlinewidth")
		tprint(0 " " Length/-2 " moveto")
		tprint(0 " " Length/2 " lineto")
		tprint("stroke")
		tprint("grestore")

		tprint("% sides of bridge")
		tprint("gsave")
		tprint("0 0.2 0 setrgbcolor")
		tprint(x " " z " translate")
		tprint(Yrot " rotate")
		#tprint("0.25 setgray")
		tprint("newpath")
		tprint(-Width/2 " " Length/-2 " moveto")
		tprint(-Width/2-10 " " Width/-2 " lineto")
		tprint(-Width/2 " " Width/-2 " lineto")
		tprint("closepath")
		tprint("fill")
		tprint("newpath")
		tprint(Width/2 " " Length/-2 " moveto")
		tprint(Width/2+10 " " Width/-2 " lineto")
		tprint(Width/2 " " Width/-2 " lineto")
		tprint("closepath")
		tprint("fill")
		tprint("newpath")
		tprint(-Width/2 " " Length/2 " moveto")
		tprint(-Width/2-10 " " Width/2 " lineto")
		tprint(-Width/2 " " Width/2 " lineto")
		tprint("closepath")
		tprint("fill")
		tprint("newpath")
		tprint(Width/2 " " Length/2 " moveto")
		tprint(Width/2+10 " " Width/2 " lineto")
		tprint(Width/2 " " Width/2 " lineto")
		tprint("closepath")
		tprint("fill")
		tprint("grestore")

	} else if (object=="pond" || object=="mound" || object=="sphere" || object=="oval" || object=="oval_shadow") {
		bprint("% " object)
		bprint("gsave")
		if (object=="pond")
			bprint("0.75 0.75 1 setrgbcolor")
		else if (object=="oval_shadow")
			bprint("0.5 setgray")
		else
			bprint(R " " G " " B " setrgbcolor")

		bprint(x " " z " translate")
		if (Width>Length) {
			bprint(Width/Length " " 1 " scale")
			Radius=Length
		} else {
			bprint(1 " " Length/Width " scale")
			Radius=Width
		}
		bprint(Yrot " rotate")
		bprint("newpath 0 0 " Radius/2 " 0 360 arc fill")
		bprint("grestore")
	} else if (object=="wall") {
		tempinit()
		Length=COS(Xrot)*Length
		Width=COS(Zrot)*Width
		tempprint("% wall")
		tempprint("gsave")
		if (R==G && G==B && B==0) {
			R=G=B=0.5
		}
		tempprint(R " " G " " B " setrgbcolor")
		tempprint(x " " z " translate")
		tempprint(Yrot " rotate")
		tempprint(Width " setlinewidth")
		#tempprint("0.5 setgray")
		tempprint("0 0 moveto")
		tempprint(0 " " Length " lineto")
		tempprint("stroke")
		tempprint("grestore")
		if (y>=0)
			tprint(tempstr)
		else
			bprint(tempstr)
	} else if (object=="curvewall") {
		if (Xrot!=180) {
			tprint("% curvewall")
			tprint("gsave")
			if (R==G && G==B && B==0) {
				R=G=B=0.5
			}
			tprint(R " " G " " B " setrgbcolor")
			#tprint("0.5 setgray")
			tprint(Width " setlinewidth")
			tprint(x " " z " " Radius-Width/2 " " Yrot " " Yrot+Angle " arc")
			tprint("stroke")
			tprint("grestore")
		} 
	} else if (object=="bank") {
		tprint("% banked curve")
		tprint("gsave")
		tprint(Width " setlinewidth")
		tprint(x " " z " " Radius-Width/2 " " Yrot " " Yrot+Angle " arc")
		tprint("stroke")
		tprint("grestore")
	} else if (object=="curveguardrail") {
		tprint("% curveguardrail")
		tprint("gsave")
		tprint("0.5 setgray")
		tprint(2 " setlinewidth")
		tprint(x " " z " " Radius-2 " " Yrot " " Yrot+Angle " arc")
		tprint("stroke")
		tprint("grestore")
	} else if (object=="flat" || object=="ice" || object=="bump") {
		bprint("% " object)
		bprint("gsave")
		if (object=="ice")
			bprint("1 setgray")
		else if (object=="bump")
			bprint("0.7 0.7 0.0 setrgbcolor")
		else
			bprint(R " " G " " B " setrgbcolor")
		#bprint("0.5 setgray")
		bprint(x-Width/2 " " z-Length/2 " translate")
		bprint(Yrot " rotate")
		bprint("newpath")
		bprint("0 0 moveto")
		bprint(0  " " Length " lineto")
		bprint(Width " " Length " lineto")
		bprint(Width " " 0 " lineto")
		bprint("closepath")
		bprint("fill")
		bprint("grestore")
	} else if (object=="generic" || object=="left" || object =="right" || object=="two") {
		# generic sign
		# left curve sign
		# right curve sign
		# two pole sign
		tprint("% generic sign")
		tprint("gsave")
		if (R==G && G==B && B==0) {
			R=G=B=1
		}
		tprint(R " " G " " B " setrgbcolor")
		tprint(x " " z " translate")
		tprint(Yrot " rotate")
		tprint(Width " setlinewidth")
		tprint("newpath")
		tprint("0 0 moveto")
		tprint("0 5 lineto")
		tprint("stroke")
		tprint("grestore")
	} else if (object=="lower") {
		# lower tube
		tprint("% lower tube")
		tprint("gsave")
		tprint("0.25 setgray")
		tprint(x " " z " translate")
		tprint(Yrot " rotate")
		tprint(Radius*2 " setlinewidth")
		tprint(0 " " Length/-2 " moveto")
		tprint(0 " " Length/2 " lineto")
		tprint("stroke")
		tprint("grestore")
	} else if (object=="start" || object=="finish" ||object=="checkpoint") {
		vtprint("% " object)
		vtprint("gsave")
		vtprint(x " " z " translate")
		vtprint(Yrot " rotate")
		if (object=="start")
			vtprint("0.75 1 0.75 setrgbcolor")
		else if (object=="finish")
			vtprint("1 0.75 0.75 setrgbcolor")
		else
			vtprint("1 setgray")
		vtprint("0 0 30 9 360 arc fill")
		vtprint("0 setgray")
		vtprint("0 0 30 9 360 arc stroke")
		vtprint("/Helvetica findfont")
		vtprint("30 scalefont setfont")
		split(Type,a)
		if (object=="start") a[2]="S"
		else if (object=="finish") a[2]="F"
		vtprint("(" a[2] ") stringwidth pop -2 div -10 moveto")
		vtprint("(" a[2] ") show")
		vtprint("grestore")
	} else if (object=="hill") {
		for (i=0;i<4;i++) randomhill()

	} else if (object=="wormhole") {
		# don't bother

	} else if (object=="parking") {
		tprint("% parking lot")
		tprint("gsave")
		tprint(x " " z " translate")
		tprint(Yrot " rotate")
		tprint(Width " setlinewidth")
		tprint("newpath")
		tprint("0 " Length/-2 " moveto")
		tprint("0 " Length/2 " lineto")
		tprint("stroke")
		tprint("grestore")

	} else if (object=="railroad") {
		if (Width==40) Width=6
		Length=COS(Xrot)*Length
		Width=COS(Zrot)*Width
		tprint("% " object)
		tprint("gsave")
		tprint(x " " z " translate")
		tprint(Yrot " rotate")
		tprint("newpath")
		tprint(Width/-2 " 0 moveto")
		tprint(Width/-2 " " Length " lineto")
		tprint("stroke")
		tprint(Width/2 " 0 moveto")
		tprint(Width/2 " " Length " lineto")
		tprint("stroke")
		tprint("0 5 " Length " { dup " Width/-2 " exch moveto " Width/2 " exch lineto stroke} for")
		tprint("grestore")

	} else if (object=="pillar" || object=="cylinder") {
		# don't bother

	} else if (object=="tree" || object=="conifer" || object=="deciduous") {
		tprint("% " object)
		tprint("gsave")
		tprint("0 0.8 0 setrgbcolor")
		#tprint("0.6 setgray")
		tprint("newpath " x " " z " 10 0 360 arc fill")
		tprint("grestore")

	} else if (object=="forest") {
		tprint("% forest")
		tprint("gsave")
		tprint("0 0.8 0 setrgbcolor")
		#tprint("0.6 setgray")
		for (i=0;i<Count;i++) {
			xx=rand*Width
			zz=rand*Length
			tprint("newpath " xx " " zz " 10 0 360 arc fill")
		}
		tprint("grestore")

	} else if (object=="weed" || object=="grass" ||object=="lawn") {
		bprint("% " object)
		bprint("gsave")
		bprint("0 0.9 0 setrgbcolor")
		#bprint("0.8 setgray")
		bprint(x " " z " translate")
		bprint(Yrot " rotate")
		bprint(Width " setlinewidth")
		bprint("newpath")
		bprint("0 0 moveto")
		bprint(0 " " Length " lineto")
		bprint("stroke")
		bprint("grestore")

	} else if (object=="bush") {
		tprint("% " object)
		tprint("gsave")
		tprint("0 0.8 0 setrgbcolor")
		#tprint("0.6 setgray")
		tprint("newpath " x " " z " 5 0 360 arc fill")
		tprint("grestore")

	} else if (object=="skyscraper") {
		tprint("% skyscraper")
		tprint("gsave")
		tprint("0.15 setgray")
		tprint(x " " z " translate")
		tprint(Yrot " rotate")
		tprint(Length " setlinewidth")
		tprint("newpath")
		tprint(Length/2 " 0 moveto")
		tprint(Length/2 " " Width " lineto")
		tprint("stroke")
		tprint("grestore")
		
	} else if (object=="silo") {
		tprint("% " object)
		tprint("gsave")
		tprint("0.7 0 0 setrgbcolor")
		#tprint("0.15 setgray")
		tprint("newpath " x " " z " 10 0 360 arc fill")
		tprint("grestore")

	} else if (object=="house" || object=="barn") {
		tprint("% " object)
		tprint("gsave")
		if (object==barn)
			tprint("0.7 0 0 setrgbcolor")
		else
			tprint("0.15 setgray")
		tprint(x " " z " translate")
		tprint(Yrot " rotate")
		tprint("40 setlinewidth")
		tprint("newpath")
		tprint("0 -20 moveto")
		tprint("0 20 lineto")
		tprint("stroke")
		tprint("grestore")
	}
	if ($0~/^Object:/) {
		object = $2
		Yrot=0
		Xrot=0
		Zrot=0
		Width=40
		Length=100
		Height=0
		Radius=0
		Angle=180
		x=0
		y=0
		z=0
		R=0
		G=0
		B=0
		Count=0
		tprint("")
		#tprint("newpath")
		next
	} else {
		object = "none"
	}
}
/^Scene Location:/ {
	if (dirty) {
	    if (scenex>=s_xmin && scenex<=s_xmax && scenez>=s_zmin && scenez<=s_zmax) {
		if (show_grid) {
			tprint("gsave")
			tprint("[10 10] 0 setdash")
			tprint("grid stroke grestore")
			tprint("border stroke axis stroke")
		}
		if (single_scene) {
			for (i=0;i<=sh;i++) print sceneheader[i]
			for (i=0;i<=sb;i++) print scenebottom[i]
			for (i=0;i<=st;i++) print scenetop[i]
			for (i=0;i<=svt;i++) print scenevtop[i]
			print "showpage"
		} else {
			S_x[sn]=scenex
			S_z[sn]=scenez
			for (i=0;i<=st;i++) S_top[sn,i]=scenetop[i]
			for (i=0;i<=svt;i++) S_vtop[sn,i]=scenevtop[i]
			for (i=0;i<=sb;i++) S_bottom[sn,i]=scenebottom[i]
			S_st[sn]=st
			S_svt[sn]=svt
			S_sb[sn]=sb
			sn++
		}
	    }
	    if (single_scene) {
		newpage=1
	    }
	}
	scenex = $3
	scenez = $4
	if (newpage) {
		sh=0; sceneheader[sh]=""

		hprint("%!")
		hprint("% function to draw the x and z axis")
		hprint("/axis {newpath")
		hprint("-1000 0 moveto 1000 0 lineto")
		hprint("0 -1000 moveto 0 1000 lineto")
		hprint("} def")
		hprint("")
		hprint("% function to draw a grid")
		hprint("/grid {newpath")
		hprint("-1000 100 1000 { 1 copy -1000 moveto 1000 lineto } for")
		hprint("-1000 100 1000 { 1 copy -1000 exch moveto 1000 exch lineto } for")
		hprint("} def")
		hprint("")
		hprint("% function to define a scene boundary")
		hprint("/border {newpath")
		hprint("-1000 1000 moveto")
		hprint("1000 1000 lineto")
		hprint("1000 -1000 lineto")
		hprint("-1000 -1000 lineto")
		hprint("closepath")
		hprint("} def")
		hprint("")
		if (flabel)
			LABEL=FILENAME
		if (clabel)
			CLABEL=" (x=" scenex ", z=" scenez ")"
		if (llabel || flabel) {
			hprint("gsave")
			hprint("/Helvetica findfont")
			hprint("30 scalefont setfont")
			hprint("(" LABEL CLABEL ") stringwidth pop -2 div 24 moveto")
			hprint("(" LABEL CLABEL ") show")
			hprint("grestore")
		}
		hprint("% Set the coordinate system so that the center of the page is 0,0")
		hprint("306 396 translate")
		hprint("% set the coordinate system so that we have a range of [-1000,1000]")
		hprint("0.25 0.25 scale")
		hprint("")
		newpage=0
	}
	svt=0; scenevtop[svt]=""
	st=0; scenetop[st]=""
	sb=0; scenebottom[sb]=""
	if (draw_ground)
		bprint("gsave 0.88 1 0.63 setrgbcolor border fill grestore")
	if (show_borders) {
		bprint("gsave 10 setlinewidth border stroke grestore")
	}
	bprint("")
	xmax=MAX(xmax,scenex)
	zmax=MAX(zmax,scenez)
	xmin=MIN(xmin,scenex)
	zmin=MIN(zmin,scenez)
	object="none"
	dirty=1
	Yrot=0
	Xrot=0
	Zrot=0
	Width=40
	Length=100
	Height=0
	Radius=0
	Angle=180
	Count=0
	x=0
	y=0
	z=0
	R=0
	G=0
	B=0

	# Add code here to print out filename and scene location
}
/^Scene Type:/ {
	type=$3 $4 $5 $6 $7 $8
	if (draw_ground)
		bprint("gsave 0.88 1 0.63 setrgbcolor border fill grestore")
	bprint("")
	tprint("% scene type = " type)
	if (type=="flatleftt") {
		tprint("gsave")
		tprint("newpath")
		tprint("0 -1000 moveto")
		tprint("40 setlinewidth")
		tprint("0 1000 lineto")
		tprint("stroke")
		tprint("newpath")
		tprint("0 0 moveto")
		tprint("40 setlinewidth")
		tprint("-1000 0 lineto")
		tprint("stroke")
		tprint("grestore")

	} else if (type=="flatrightt") {
		tprint("gsave")
		tprint("newpath")
		tprint("0 -1000 moveto")
		tprint("40 setlinewidth")
		tprint("0 1000 lineto")
		tprint("stroke")
		tprint("newpath")
		tprint("0 0 moveto")
		tprint("40 setlinewidth")
		tprint("1000 0 lineto")
		tprint("stroke")
		tprint("grestore")

	} else if (type=="flatupt") {
		tprint("gsave")
		tprint("newpath")
		tprint("-1000 0 moveto")
		tprint("30 setlinewidth")
		tprint("1000 0 lineto")
		tprint("stroke")
		tprint("newpath")
		tprint("0 1000 moveto")
		tprint("40 setlinewidth")
		tprint("0 0 lineto")
		tprint("stroke")
		tprint("grestore")

	} else if (type=="flatdownt") {
		tprint("gsave")
		tprint("newpath")
		tprint("-1000 0 moveto")
		tprint("30 setlinewidth")
		tprint("1000 0 lineto")
		tprint("stroke")
		tprint("newpath")
		tprint("0 -1000 moveto")
		tprint("40 setlinewidth")
		tprint("0 0 lineto")
		tprint("stroke")
		tprint("grestore")

	} else if (type=="flatturnrightdown" || type=="flatturnrightup" || type=="flatturnleftdown" || type=="flatturnleftup") {
		tprint("gsave")
		tprint("30 setlinewidth")
		tprint("newpath")
		if (type=="flatturnrightdown")
			tprint("1000 -1000 1000 90 180 arc")
		else if (type=="flatturnrightup")
			tprint("1000 1000 1000 180 270 arc")
		else if (type=="flatturnleftdown")
			tprint("-1000 -1000 1000 0 90 arc")
		else if (type=="flatturnleftup")
			tprint("-1000 1000 1000 -90 0 arc")
		tprint("stroke")
		tprint("grestore")

	} else if (type=="flatxhighway" || type=="flatxroad" || type=="hillxroad") {
		tprint("gsave")
		tprint("newpath")
		tprint("-1000 0 moveto")
		if (type=="flatxhighway")
			tprint("60 setlinewidth")
		else
			tprint("30 setlinewidth")
		tprint("1000 0 lineto")
		tprint("stroke")
		tprint("grestore")
		if (type=="hillxroad")
			for (i=0;i<4;i++) randomhill()

	} else if (type=="flatzhighway" || type=="flatzroad" || type=="hillzroad") {
		tprint("gsave")
		tprint("newpath")
		tprint("0 -1000 moveto")
		if (type=="flatzhighway")
			tprint("60 setlinewidth")
		else
			tprint("30 setlinewidth")
		tprint("0 1000 lineto")
		tprint("stroke")
		tprint("grestore")
		if (type=="hillzroad")
			for (i=0;i<4;i++) randomhill()
			

	} else if (type=="flatxhighwayoverpass") {
		tprint("gsave")
		tprint("gsave")
		tprint("0 0.2 0 setrgbcolor")
		#tprint("0.25 setgray")
		tprint("newpath")
		tprint("-100 30 moveto")
		tprint("0 50 lineto")
		tprint("100 30 moveto")
		tprint("closepath")
		tprint("fill")
		tprint("newpath")
		tprint("-100 -30 moveto")
		tprint("0 50 lineto")
		tprint("100 -30 moveto")
		tprint("closepath")
		tprint("fill")
		tprint("grestore")
		tprint("newpath")
		tprint("-1000 0 moveto")
		tprint("60 setlinewidth")
		tprint("1000 0 lineto")
		tprint("stroke")
		tprint("newpath")
		tprint("0 -1000 moveto")
		tprint("60 setlinewidth")
		tprint("0 1000 lineto")
		tprint("stroke")
		tprint("grestore")

	} else if (type=="flatzhighwayoverpass") {
		tprint("gsave")
		tprint("gsave")
		tprint("0 0.2 0 setrgbcolor")
		#tprint("0.25 setgray")
		tprint("newpath")
		tprint("-30 -100 moveto")
		tprint("-50 0lineto")
		tprint("-30 100 moveto")
		tprint("closepath")
		tprint("fill")
		tprint("newpath")
		tprint("30 -100 moveto")
		tprint("50 0 lineto")
		tprint("30 100 moveto")
		tprint("closepath")
		tprint("fill")
		tprint("grestore")
		tprint("newpath")
		tprint("-1000 0 moveto")
		tprint("60 setlinewidth")
		tprint("1000 0 lineto")
		tprint("stroke")
		tprint("newpath")
		tprint("0 -1000 moveto")
		tprint("60 setlinewidth")
		tprint("0 1000 lineto")
		tprint("stroke")
		tprint("grestore")

	} else if (type=="hillnoroads") {
		for (i=0;i<4;i++) randomhill()

	} else if (type=="flatcrossroads" || type=="hillcrossroads" || type=="flatoverpass") {
		# The flatoverpass is random, so we just draw it as an
		# intersection
		if (type=="hillcrossroads")
			for (i=0;i<4;i++) randomhill()
		tprint("gsave")
		tprint("newpath")
		tprint("-1000 0 moveto")
		tprint("30 setlinewidth")
		tprint("1000 0 lineto")
		tprint("stroke")
		tprint("newpath")
		tprint("0 -1000 moveto")
		tprint("0 1000 lineto")
		tprint("stroke")
		tprint("grestore")
	} else if (type=="flatnoroads") {
		# do nothing
	} else if (type=="village" || type=="town" || type=="city") {
		# just draw them as crossroads
		tprint("gsave")
		tprint("newpath")
		tprint("-1000 0 moveto")
		tprint("30 setlinewidth")
		tprint("1000 0 lineto")
		tprint("stroke")
		tprint("newpath")
		tprint("0 -1000 moveto")
		tprint("0 1000 lineto")
		tprint("stroke")
		tprint("grestore")
	} else if (type=="whateverfits") {
		# yeah, like this is going to occur...
	}
}
/^Data:/ {
	debug()
	num_points=$2
	dimension=$3
	idx=0
	next
}
$1 ~ /^[-.0-9]/ {
	debug()
	idx++
	for (p=1;p<=NF;p++)
		spline[idx,p]=$p
	next
}
/^Width:/ {
	debug()
	Width=$2
	next
}
/^Length:/ {
	debug()
	Length=$2
	next
}
/^Height:/ {
	debug()
	Height=$2
	next
}
/^Radius:/ {
	debug()
	Radius=$2
	next
}
/^Angle:/ {
	debug()
	Angle=$2
	next
}
/^Yrot:/ {
	debug()
	Yrot=$2
	next
}
/^Xrot:/ {
	debug()
	Xrot=$2
	next
}
/^Zrot:/ {
	debug()
	Zrot=$2
	next
}
/^Position:/ {
	debug()
	x=$2
	y=$3
	z=$4
	next
}
/^X:/ {
	debug()
	x=$2
	next
}
/^Y:/ {
	debug()
	y=$2
	next
}
/^Z:/ {
	debug()
	z=$2
	next
}
/^Color:/ {
	debug()
	R=$2
	G=$3
	B=$4
	next
}
/^R:/ {
	debug()
	R=$2
	next
}
/^G:/ {
	debug()
	G=$2
	next
}
/^B:/ {
	debug()
	B=$2
	next
}
/^Size:/ {
	debug()
	Length=$2
	Width=$3
	Height=$4
	next
}
/^Spacing:/ {
	debug()
	Spacing=$2
	next
}
/^Type:/ {
	debug()
	Type = $2 " " $3
	next
}
{
	debug()
	if (DEBUG==2)
		tprint("% misc line:" $0 ", NF=" NF)
}

END {
	if (dirty) {
		if (show_grid) {
			tprint("gsave")
			tprint("[10 10] 0 setdash")
			tprint("grid stroke grestore")
			tprint("border stroke axis stroke")
		}
		if (single_scene && scenex>=s_xmin && scenex<=s_xmax &&
		    scenez>=s_zmin && scenez<=s_zmax) {
			for (i=0;i<=sh;i++) print sceneheader[i]
			for (i=0;i<=sb;i++) print scenebottom[i]
			for (i=0;i<=st;i++) print scenetop[i]
			for (i=0;i<=svt;i++) print scenevtop[i]
			print "showpage"
		} else {
			print "%!"
			S_x[sn]=scenex
			S_z[sn]=scenez
			for (i=0;i<=st;i++) S_top[sn,i]=scenetop[i]
			for (i=0;i<=svt;i++) S_vtop[sn,i]=scenevtop[i]
			for (i=0;i<=sb;i++) S_bottom[sn,i]=scenebottom[i]
			S_st[sn]=st
			S_svt[sn]=svt
			S_sb[sn]=sb
			sn++
			xmax=MIN(xmax,s_xmax)
			zmax=MIN(zmax,s_zmax)
			xmin=MAX(xmin,s_xmin)
			zmin=MAX(zmin,s_zmin)
			xscenes=xmax-xmin+1
			zscenes=zmax-zmin+1
			print "% xscenes=" xscenes
			print "% zscenes=" zscenes
			if (flabel) LABEL=FILENAME
			if (clabel) {
				if (xmin!=xmax && zmin!=zmax)
					CLABEL = " (x=[" xmin "," xmax "], z=[" zmin "," zmax "])"
				else if (xmin!=xmax)
					CLABEL = " (x=[" xmin "," xmax "], z=" zmax ")"
				else if (zmin!=zmax)
					CLABEL = " (x=" xmax ", z=[" zmin "," zmax "])"
				else
					CLABEL = " (x=" xmax ", z=" zmax ")"
			}
			if (llabel || flabel) {
				print "/Helvetica findfont"
				print "30 scalefont setfont"
			}
			if (xscenes>zscenes) {
				rotated=1
				longaxis=xscenes
				shortaxis=zscenes
				if (llabel || flabel) {
					print "gsave"
					print "(" LABEL CLABEL ") stringwidth pop 2 div 396 add 24 exch moveto"
					print "-90 rotate"
					print "(" LABEL CLABEL ") show"
					print "grestore"
				}
			} else {
				longaxis=zscenes
				shortaxis=xscenes
				if (llabel || flabel) {
					print "gsave"
					print "(" LABEL CLABEL ") stringwidth pop -2 div 306 add 24 moveto"
					print "(" LABEL CLABEL ") show"
					print "grestore"
				}
			}
			print "% function to draw the x and z axis"
			print "/axis {newpath"
			print "-1000 0 moveto 1000 0 lineto"
			print "0 -1000 moveto 0 1000 lineto"
			print "} def"
			print ""
			print "% function to draw a grid"
			print "/grid {newpath"
			print "-1000 100 1000 { 1 copy -1000 moveto 1000 lineto } for"
			print "-1000 100 1000 { 1 copy -1000 exch moveto 1000 exch lineto } for"
			print "} def"
			print ""
			print "% function to define a scene boundary"
			print "/border {newpath"
			print "-1000 1000 moveto"
			print "1000 1000 lineto"
			print "1000 -1000 lineto"
			print "-1000 -1000 lineto"
			print "closepath"
			print "} def"
			print ""
			print "% Set the coordinate system so that the center of the page is 0,0"
			zunits=3024
			xunits=2304
			if (llabel || flabel) {
				if (rotated) {
					print "324 396 translate"
					xunits=2160
				} else {
					print "306 414 translate"
					zunits=2880
				}
			} else {
				print "306 396 translate"
			}
			print "% set the coordinate system so that we have a range of [-1000,1000]"
			print "0.25 0.25 scale"
			print ""
			if (rotated)
				print "-90 rotate"
			if ((longaxis*xunits)/zunits>=shortaxis)
				print(zunits/(longaxis*2000) " " zunits/(longaxis*2000) " scale")
			else
				print(xunits/(shortaxis*2000) " " xunits/(shortaxis*2000) " scale")
			centerx=(xmax+xmin)*1000
			centerz=(zmax+zmin)*1000
			print "% centerx=" centerx
			print "% centerz=" centerz

			for (i=0;i<sn;i++) {
			    if (S_x[i]>=s_xmin && S_x[i]<=s_xmax && S_z[i]>=s_zmin && S_z[i]<=s_zmax) {
				print "gsave"
				print "% ======= scene " S_x[i] " " S_z[i] " ======"
				print S_x[i]*2000-centerx " " S_z[i]*2000-centerz " translate"
				print "-1000 -1000 2000 2000 rectclip"
				for (j=0;j<=S_sb[i];j++) print S_bottom[i,j]
				for (j=0;j<=S_st[i];j++) print S_top[i,j]
				for (j=0;j<=S_svt[i];j++) print S_vtop[i,j]
				print "grestore"
			    }
			}
			print "showpage"
		}
	}
}
_EOF_

args=$*
if [ $sf != 1 ]; then
	args="'$args'"
fi

eval set -- "$args"
while [ $# != 0 ]; do
	awk	-vsingle_scene=$ss \
		-vshow_borders=$sb \
		-vshow_grid=$sg \
		-vdraw_ground=$draw_ground \
		-vs_xmin=$xmin \
		-vs_xmax=$xmax \
		-vs_zmin=$zmin \
		-vs_zmax=$zmax \
		-vflabel=$flabel \
		-vllabel=$llabel \
		-v"LABEL=$LABEL" \
		-vclabel=$sc \
		-vDEBUG=$debug \
		-f /tmp/Drive2PS$$ \
		$1
	shift
done

rm /tmp/Drive2PS$$
