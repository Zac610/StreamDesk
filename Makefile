.PHONY: clean All

All:
	@echo "----------Building project:[ StreamDeskMain - Debug ]----------"
	@cd "StreamDeskMain" && "$(MAKE)" -f  "StreamDeskMain.mk"
clean:
	@echo "----------Cleaning project:[ StreamDeskMain - Debug ]----------"
	@cd "StreamDeskMain" && "$(MAKE)" -f  "StreamDeskMain.mk" clean
