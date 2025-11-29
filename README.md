```sh
# Clone or download the extension source
make
sudo make install
```

```sql
CREATE EXTENSION base32;

-- Encode text to Base32
SELECT base32_encode('Hello Moto!');
-- Result: 'JBSWY3DPEBGW65DPEE======'

-- Encode binary data
SELECT base32_encode('\x48656c6c6f204d6f746f21'::bytea);
-- Result: 'JBSWY3DPEBGW65DPEE======'

-- Decode back to binary
SELECT base32_decode('JBSWY3DPEBGW65DPEE======');
-- Result: '\x48656c6c6f204d6f746f21'

-- Decode directly to text
SELECT base32_decode_to_text('JBSWY3DPEBGW65DPEE======');
-- Result: 'Hello Moto!'

----------------------------------------------------------------

-- Encode with specific encoding
SELECT base32_encode('Café', 'UTF8');
SELECT base32_encode('Hello', 'LATIN1');

-- Decode with specific encoding
SELECT base32_decode_to_text('INQXI2JP', 'UTF8');
SELECT base32_decode_to_text('NBSWY3DP', 'LATIN1');

----------------------------------------------------------------

-- Valid Base32 strings
SELECT base32_is_valid('JBSWY3DPEBLW64TMMQQQ===='); -- true
SELECT base32_is_valid('NBSWY3DP'); -- true
SELECT base32_is_valid('abcdefg234567'); -- true (case insensitive)
SELECT base32_is_valid('MZXW6YTBOI======'); -- true

-- Invalid Base32 strings
SELECT base32_is_valid('Invalid!@#'); -- false (invalid characters)
SELECT base32_is_valid('ABC123'); -- false (wrong length)
SELECT base32_is_valid('AB=Cdefg'); -- false (padding in middle)
SELECT base32_is_valid('ABCDEFG1'); -- false ('1' not in alphabet)

```

