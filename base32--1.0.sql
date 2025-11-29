-- =====================================================
-- Base32 Extension (RFC 4648)
-- =====================================================

CREATE FUNCTION base32_encode(data bytea)
RETURNS text
AS 'MODULE_PATHNAME', 'base32_encode_pg'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION base32_encode(txt text)
RETURNS text
AS $$
    SELECT base32_encode(convert_to($1, 'UTF8'));
$$ LANGUAGE sql IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION base32_encode(txt text, encoding text)
RETURNS text
AS $$
    SELECT base32_encode(convert_to($1, $2));
$$ LANGUAGE sql IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION base32_decode(txt text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'base32_decode_pg'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION base32_decode_to_text(txt text, encoding text DEFAULT 'UTF8')
RETURNS text
AS $$
    SELECT convert_from(base32_decode($1), $2);
$$ LANGUAGE sql IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION base32_is_valid(txt text)
RETURNS boolean
AS 'MODULE_PATHNAME', 'base32_is_valid_pg'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

COMMENT ON FUNCTION base32_encode(bytea) IS 'Base32 encode binary data (RFC 4648)';
COMMENT ON FUNCTION base32_encode(text) IS 'Base32 encode text (UTF-8)';
COMMENT ON FUNCTION base32_encode(text, text) IS 'Base32 encode text with specified encoding';
COMMENT ON FUNCTION base32_decode(text) IS 'Base32 decode to binary data';
COMMENT ON FUNCTION base32_decode_to_text(text, text) IS 'Base32 decode to text with specified encoding';
COMMENT ON FUNCTION base32_is_valid(text) IS 'Validate Base32 string (RFC 4648)';
