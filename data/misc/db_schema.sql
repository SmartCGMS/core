--
-- File generated with SQLiteStudio v3.1.1 on po øíj 12 09:24:59 2020
--
-- Text encoding used: System
--
PRAGMA foreign_keys = off;
BEGIN TRANSACTION;

-- Table: diagnosis
CREATE TABLE diagnosis (
    id        INTEGER PRIMARY KEY AUTOINCREMENT,
    E08       BOOLEAN NOT NULL,
    E09       BOOLEAN NOT NULL,
    E10       BOOLEAN NOT NULL,
    E11       BOOLEAN NOT NULL,
    O24       BOOLEAN NOT NULL,
    other     BOOLEAN NOT NULL,
    complete  TEXT    NOT NULL,
    segmentid BIGINT,
    CONSTRAINT fk_diagnosis_segmentid FOREIGN KEY (
        segmentid
    )
    REFERENCES timesegment (id) DEFERRABLE INITIALLY DEFERRED
);


-- Table: difuse2params
CREATE TABLE difuse2params (
    id        INTEGER            PRIMARY KEY AUTOINCREMENT,
    p         [DOUBLE PRECISION],
    cg        [DOUBLE PRECISION],
    c         [DOUBLE PRECISION],
    dt        [DOUBLE PRECISION],
    h         [DOUBLE PRECISION],
    k         [DOUBLE PRECISION],
    s         [DOUBLE PRECISION],
    segmentid BIGINT,
    CONSTRAINT fk_difuse2params_segmentid FOREIGN KEY (
        segmentid
    )
    REFERENCES timesegment (id) DEFERRABLE INITIALLY DEFERRED
);


-- Table: difuse3params
CREATE TABLE difuse3params (
    id        INTEGER            PRIMARY KEY AUTOINCREMENT,
    p         [DOUBLE PRECISION],
    cg        [DOUBLE PRECISION],
    c         [DOUBLE PRECISION],
    tau       [DOUBLE PRECISION],
    dt        [DOUBLE PRECISION],
    h         [DOUBLE PRECISION],
    kpos      [DOUBLE PRECISION],
    kneg      [DOUBLE PRECISION],
    segmentid BIGINT,
    CONSTRAINT fk_difuse3params_segmentid FOREIGN KEY (
        segmentid
    )
    REFERENCES timesegment (id) DEFERRABLE INITIALLY DEFERRED
);


-- Table: difuseparams
CREATE TABLE difuseparams (
    id        INTEGER            PRIMARY KEY AUTOINCREMENT,
    p         [DOUBLE PRECISION],
    cg        [DOUBLE PRECISION],
    c         [DOUBLE PRECISION],
    dt        [DOUBLE PRECISION],
    segmentid BIGINT,
    CONSTRAINT fk_difuseparams_segmentid FOREIGN KEY (
        segmentid
    )
    REFERENCES timesegment (id) DEFERRABLE INITIALLY DEFERRED
);


-- Table: measuredvalue
CREATE TABLE measuredvalue (
    id            INTEGER            PRIMARY KEY AUTOINCREMENT,
    measuredat    TEXT,
    blood         [DOUBLE PRECISION],
    ist           [DOUBLE PRECISION],
    isig          [DOUBLE PRECISION],
    insulin       [DOUBLE PRECISION],
    carbohydrates [DOUBLE PRECISION],
    calibration   [DOUBLE PRECISION],
    segmentid     BIGINT,
    CONSTRAINT fk_measuredvalue_segmentid FOREIGN KEY (
        segmentid
    )
    REFERENCES timesegment (id) DEFERRABLE INITIALLY DEFERRED
);


-- Table: parallel_segment
CREATE TABLE parallel_segment (
    id    INTEGER PRIMARY KEY AUTOINCREMENT,
    dummy INTEGER NOT NULL
);


-- Table: segment_source
CREATE TABLE segment_source (
    id        INTEGER PRIMARY KEY AUTOINCREMENT,
    filepath  TEXT    NOT NULL,
    device    TEXT    NOT NULL,
    segmentid BIGINT,
    CONSTRAINT fk_segment_source_segmentid FOREIGN KEY (
        segmentid
    )
    REFERENCES timesegment (id) DEFERRABLE INITIALLY DEFERRED
);


-- Table: steilrebrinparams
CREATE TABLE steilrebrinparams (
    id        INTEGER            PRIMARY KEY AUTOINCREMENT,
    t         [DOUBLE PRECISION],
    g         [DOUBLE PRECISION],
    alpha     [DOUBLE PRECISION],
    beta      [DOUBLE PRECISION],
    gamma     [DOUBLE PRECISION],
    segmentid BIGINT,
    CONSTRAINT fk_steilrebrinparams_segmentid FOREIGN KEY (
        segmentid
    )
    REFERENCES timesegment (id) DEFERRABLE INITIALLY DEFERRED
);


-- Table: subject
CREATE TABLE subject (
    id       INTEGER PRIMARY KEY AUTOINCREMENT,
    name     TEXT,
    comments TEXT,
    sex      INTEGER,
    weight   INTEGER,
    user_id  BIGINT
);


-- Table: timesegment
CREATE TABLE timesegment (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    name        TEXT,
    comment     TEXT,
    subjectid   BIGINT,
    parallel_id BIGINT,
    deleted     BOOLEAN,
    CONSTRAINT fk_timesegment_subjectid FOREIGN KEY (
        subjectid
    )
    REFERENCES subject (id) DEFERRABLE INITIALLY DEFERRED,
    CONSTRAINT fk_timesegment_parallel FOREIGN KEY (
        parallel_id
    )
    REFERENCES parallel_segment (id) DEFERRABLE INITIALLY DEFERRED
);


COMMIT TRANSACTION;
PRAGMA foreign_keys = on;
