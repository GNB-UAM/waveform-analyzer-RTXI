PLUGIN_NAME = waveform_analyzer

HEADERS = waveform-analyzer.h

SOURCES = waveform-analyzer.cpp\
          moc_waveform-analyzer.cpp\

LIBS = 

### Do not edit below this line ###

include $(shell rtxi_plugin_config --pkgdata-dir)/Makefile.plugin_compile
