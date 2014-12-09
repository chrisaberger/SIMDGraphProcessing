DROP TABLE IF EXISTS edge CASCADE;
CREATE TABLE edge(a BIGINT, b BIGINT);

COPY edge FROM :'dataset' DELIMITER ' ';

CREATE INDEX edge_a_index ON edge(a);
CREATE INDEX edge_b_index ON edge(b);

ANALYZE VERBOSE edge;
