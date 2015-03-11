DROP TABLE IF EXISTS edge;
DROP TABLE IF EXISTS edgeRaw;
DROP TABLE IF EXISTS uedge;

DROP TABLESPACE ramspace;
CREATE TABLESPACE ramspace LOCATION '/dev/shm/noetzli/postgresql/data';

CREATE TABLE edgeRaw(a BIGINT, b BIGINT) TABLESPACE ramspace;
CREATE TABLE edge(a BIGINT, b BIGINT) TABLESPACE ramspace;
CREATE TABLE uedge(a BIGINT, b BIGINT) TABLESPACE ramspace;

COPY edgeRaw FROM :'dataset' DELIMITER ' ';

-- Pruned edge relation
INSERT INTO edge(a, b) (
   SELECT a, b FROM edgeRaw WHERE a != b
);

CREATE INDEX edge_a_index ON edge(a);
CREATE INDEX edge_b_index ON edge(b);
ANALYZE VERBOSE edge;

-- Unpruned edge relation
INSERT INTO uedge(a, b) (
   SELECT a, b FROM edgeRaw
);

INSERT INTO uedge(a, b) (
   SELECT b, a FROM edgeRaw WHERE b != a
);

CREATE INDEX uedge_a_index ON uedge(a);
CREATE INDEX uedge_b_index ON uedge(b);
ANALYZE VERBOSE uedge;

DROP TABLE IF EXISTS edgeRaw;
