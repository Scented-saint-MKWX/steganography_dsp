import numpy as np

def xorshift32(state: int) -> tuple[int, int]:
    """
    xorshift32 PRNG (uint32).
    Returns (new_state, output).
    """
    state = int(state) & 0xFFFFFFFF
    state ^= (state << 13) & 0xFFFFFFFF
    state ^= (state >> 17) & 0xFFFFFFFF
    state ^= (state << 5) & 0xFFFFFFFF
    return state, state

def bounded_random(state: int, n: int) -> tuple[int, int]:
    """
    Bounded random in [0, n-1] using rejection sampling.
    threshold = (2^32 - n) % n
    """
    n = int(n) & 0xFFFFFFFF
    if n == 0:
        return state, 0
    # Equivalent to (-n) % n in uint32 math
    threshold = ((0xFFFFFFFF - n + 1) % n) & 0xFFFFFFFF
    
    while True:
        state, r = xorshift32(state)
        if r >= threshold:
            return state, r % n

def fisher_yates_permutation(length: int, key: int) -> np.ndarray:
    """
    Generate permutation array of size `length` seeded with `key`.
    """
    state = int(key) & 0xFFFFFFFF
    if state == 0:
        state = 1  # prevent 0 seed
    perm = np.arange(length, dtype=np.uint32)
    for i in range(length - 1, 0, -1):
        state, j = bounded_random(state, i + 1)
        perm[i], perm[j] = perm[j], perm[i]
    return perm

def inverse_permutation(perm: np.ndarray) -> np.ndarray:
    """
    Generate inverse permutation.
    """
    length = len(perm)
    inv = np.zeros(length, dtype=np.uint32)
    inv[perm] = np.arange(length, dtype=np.uint32)
    return inv

def key_check_hash(key: int) -> int:
    """
    Simple mixing function for key check.
    """
    h = int(key) & 0xFFFFFFFF
    h ^= (h >> 16) & 0xFFFFFFFF
    h = (h * 0x45d9f3b) & 0xFFFFFFFF
    h ^= (h >> 16) & 0xFFFFFFFF
    return h

if __name__ == '__main__':
    # Simple test
    s = 1
    for _ in range(5):
        s, v = xorshift32(s)
        print(f"Random: {hex(v)}")
