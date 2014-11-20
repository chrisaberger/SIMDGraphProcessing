DROP TABLE IF EXISTS edge;
DROP TABLE IF EXISTS node;
CREATE TABLE edgeRaw(a BIGINT, b BIGINT, year INT);
CREATE TABLE edge(a BIGINT, b BIGINT, year INT);
CREATE TABLE node(a BIGINT, place INT);
CREATE INDEX ON edge(a);

COPY edgeRaw FROM :'edge_dataset';
COPY node FROM :'attr_dataset';

INSERT INTO edge(a, b, year) (
   SELECT a, b, year FROM edgeRaw WHERE a < b
);

INSERT INTO edge(a, b, year) (
   SELECT b, a, year FROM edgeRaw WHERE a > b
);

DROP TABLE IF EXISTS edgeRaw;
