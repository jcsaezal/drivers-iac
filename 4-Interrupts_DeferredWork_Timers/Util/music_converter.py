#!/usr/bin/env python
import pandas as pd
import sys

if len(sys.argv)!=3:
	print("Usage: %s <frequency_table> <music_file>" % sys.argv[0])
	exit(1)

try:
	ft=pd.read_csv(sys.argv[1])
except Exception as e:
	print("Error when opening frequency table",file=sys.stderr)
	print(e,file=sys.stderr)
	exit(1)


try:
	sheet_music=pd.read_csv(sys.argv[2],names=["Note","Duration"])
except Exception as e:
	print("Error when opening music file",file=sys.stderr)
	print(e,file=sys.stderr)
	exit(1)


## Build a dictionary for easy translation of notes
freq={"r": 0} ## Add rest
for index, row in ft.iterrows():
	note=row.Note
	if "/" in note:
		notes=note.split("/")
	else:
		notes=[note]

	for note in notes:
		freq[note.lower()]=int(round(row.Frequency*100.0,0))


## Convert music into lower level format
sheet_music["Out"]=sheet_music.apply(lambda row: "%d:%d" % (freq[row.Note],row.Duration),axis=1)


print("music "+",".join(sheet_music["Out"].values))

