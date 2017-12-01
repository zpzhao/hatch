' play.vbS
' 播放音乐
' 2017-12-01 by zzp
set wmp=CreateObject("wmplayer.ocx")
wmp.url="D:\\ticao.mp3"
wscript.sleep 1200
wscript.sleep wmp.currentMedia.duration *1000