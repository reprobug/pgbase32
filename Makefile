EXTENSION = base32
MODULE_big = base32
OBJS = base32.o
DATA = base32--1.0.sql
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
