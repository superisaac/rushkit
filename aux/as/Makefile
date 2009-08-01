MTASC=mtasc
SWFMILL=swfmill

TGT_WIDGETSWF = ~/.ysd/swf/widget.swf
WIDGETSWF = widget.swf

# Actionscript files
WIDGET_AS= App.as

$(WIDGETSWF): main.xml widget_base.swf
	$(SWFMILL) simple main.xml $@

widget_base.swf :  $(WIDGET_AS)
	${MTASC} -v  -swf abc.swf -cp /opt/mtasc/std -cp /opt/mtasc/std8  -version 8 -trace App.mytrace -out  $@ -header 650:600:20:446688  App.as

clean:
	rm -f widget_base.swf ${WIDGETSWF} *~
