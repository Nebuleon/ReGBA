ifdef V
	CMD:=
	SUM:=@\#
else
	CMD:=@
	SUM:=@echo
endif

.PHONY: $(TARGET)
$(TARGET): $(OBJS)
	$(SUM) "  LD      $@"
	$(CMD)$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

%.o: %.c
	$(SUM) "  CC      $@"
	$(CMD)$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(SUM) "  AS      $@"
	$(CMD)$(CC) $(ASFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(SUM) "  CLEAN   ."
	$(CMD)rm -rf $(OBJS) $(TARGET) $(DATA_TO_CLEAN)
