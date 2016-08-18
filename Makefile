VERSION = 0.3b
PROJECT = ogle-${VERSION}
HOST = mfrumin@eyeres.org:/var/www/ogle/


all: doc bin src

bin: ${PROJECT}.bin.zip 

src: ${PROJECT}.src.zip 

$(PROJECT).bin.zip: $(PROJECT)
	 zip -r $(PROJECT).bin.zip $(PROJECT)

$(PROJECT): doc
	mkdir -p $(PROJECT)
	rsync -ur  LICENSE $(PROJECT)/
	rsync -ur  Release/OGLE.dll $(PROJECT)/
	rsync -ur  config/config.ini config/gliConfig_OGLE.ini $(PROJECT)/
	rsync -Cur docs  $(PROJECT)/


$(PROJECT).src.zip: $(PROJECT).src
	zip -r $(PROJECT).src.zip $(PROJECT).src;

$(PROJECT).src: doc
	mkdir -p $(PROJECT).src
	rsync -Cur Ptr  $(PROJECT).src/
	rsync -Cur mtl  $(PROJECT).src/
	rsync -Cur config  $(PROJECT).src/
	rsync -Cur docs  $(PROJECT).src/
	rsync -ur  *.h *.cpp *.dsp *.def $(PROJECT).src/
	rsync -ur  LICENSE $(PROJECT).src/


doc: docs/README.html

docs/README.html:
	cd docs && wget -pk "http://ogle.eyebeamresearch.org/readme";
	cd docs && mv ogle.eyebeamresearch.org/* .;
	cd docs && mv readme "README.html";
	cd docs && rm -rf ogle.eyebeamresearch.org/;

clean:
	rm -f $(PROJECT).zip;
	rm -f $(PROJECT).bin.zip;
	rm -rf $(PROJECT);
	rm -f $(PROJECT).src.zip;
	rm -rf $(PROJECT).src;
	cd docs && rm -rf *html files misc themes

dist: ${PROJECT}.bin.zip ${PROJECT}.src.zip
	rsync -e ssh -urv ${PROJECT}.bin.zip ${PROJECT}.src.zip ${HOST}/dist/



mtl:
	wget "http://osl.iu.edu/download/research/mtl/mtl-2.1.2-20.zip"
	unzip mtl-2.1.2-20.zip
	mv mtl-2.1.2-20/mtl .
	rm -rf mtl-2.1.2-20*
	patch mtl/mtl.h mtl.h.patch
