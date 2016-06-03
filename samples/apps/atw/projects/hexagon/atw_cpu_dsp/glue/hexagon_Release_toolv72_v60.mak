# pakman tree build file
_@ ?= @

.PHONY: tree tree_clean

tree:
	$(call job,,make V=hexagon_Release_toolv72_v60,making .)

tree_clean:
	$(call job,,make V=hexagon_Release_toolv72_v60 clean,cleaning .)

W := $(findstring ECHO,$(shell echo))# W => Windows environment
@LOG = $(if $W,$(TEMP)\\)$@-build.log

C = $(if $1,cd $1 && )$2
job = $(_@)echo $3 && ( $C )> $(@LOG) && $(if $W,del,rm) $(@LOG) || ( echo ERROR $3 && $(if $W,type,cat) $(@LOG) && $(if $W,del,rm) $(@LOG) && exit 1)
ifdef VERBOSE
  job = $(_@)echo $3 && $C
endif
