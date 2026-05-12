# TODO

- [ ] **Review performance of feature extraction**
      `features_extract()` is called on every `/fraud-score` request and
      does ISO 8601 parsing, timegm, and string scanning. Profile to see if
      it's fast enough under load, or if it needs caching / pre-parsing of
      timestamps.
