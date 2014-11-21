DROP TABLE IF EXISTS edge;
DROP TABLE IF EXISTS node;
DROP TABLE IF EXISTS edgeRaw;
DROP TABLE IF EXISTS nodeRaw;

CREATE TABLE edgeRaw(a BIGINT, b BIGINT, year TIMESTAMP, placeholder VARCHAR(1), PRIMARY KEY(a, b));
CREATE TABLE edge(a BIGINT, b BIGINT, year INT, PRIMARY KEY(a, b));
CREATE TABLE nodeRaw(a BIGINT, place INT, placeholder VARCHAR(1), PRIMARY KEY(a));
CREATE TABLE node(a BIGINT, place INT, PRIMARY KEY(a));
CREATE INDEX ON edge(a);
CREATE INDEX ON node(a);

COPY edgeRaw FROM :'edgelist' WITH DELIMITER '|';
COPY nodeRaw FROM :'attributes' WITH DELIMITER '|';

INSERT INTO edge(a, b, year) (
   SELECT DISTINCT LEAST(a, b), GREATEST(a, b), EXTRACT(YEAR FROM year) FROM edgeRaw
);

INSERT INTO node(a, place) (
   SELECT a, place FROM nodeRaw
);
