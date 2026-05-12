# TODO

- [ ] **Review performance**
      - `features_extract()` does ISO 8601 parsing, timegm, and string
        scanning on every request.
      - `fraud_detect()` brute-force scans all 3M entries with cosine
        similarity on every request (~144 ms average).
      Consider: pre-computed norms, approximate nearest neighbor (ANN),
      or reducing the dataset scan.
