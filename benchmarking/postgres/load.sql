DROP TABLE IF EXISTS edge;
CREATE TABLE edgeRaw(a BIGINT, b BIGINT);
CREATE TABLE edge(a BIGINT, b BIGINT);
COPY edgeRaw FROM :'dataset';

INSERT INTO edge(a, b) (
   SELECT a, b FROM edgeRaw WHERE a < b
);

INSERT INTO edge(a, b) (
   SELECT b, a FROM edgeRaw WHERE a > b
);

DROP TABLE IF EXISTS edgeRaw;
