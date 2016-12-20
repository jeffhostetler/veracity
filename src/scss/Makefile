TMPDIR := $(shell mktemp -d -t scsstmpXXX)
UI = ../server_files/ui
WIKI = $(UI)/modules/wiki
SRC = .

.SUFFIXES: .scss .css

$(UI)/%.css: $(SRC)/%.scss
	sass --unix-newlines --scss --no-cache --style expanded $< $(TMPDIR)/t.css
	perl tabify < $(TMPDIR)/t.css > $@

$(WIKI)/%.css: $(SRC)/modules/wiki/%.scss
	sass --unix-newlines --scss --no-cache --style expanded $< $(TMPDIR)/t.css
	perl tabify < $(TMPDIR)/t.css > $@

all:	$(UI)/sg.css $(UI)/uitweak.css $(WIKI)/module.css

$(UI)/sg.css:	$(SRC)/sg.scss $(SRC)/_defs.scss $(SRC)/_containers.scss

$(UI)/uitweak.css:	$(SRC)/uitweak.scss $(SRC)/_defs.scss

$(WIKI)/module.css:	$(SRC)/modules/wiki/module.scss $(SRC)/_defs.scss

watch:
	sass --unix-newlines --no-cache --scss --style expanded  --watch $(SRC):$(UI)
