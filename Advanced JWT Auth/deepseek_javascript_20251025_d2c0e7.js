const jwt = require('jsonwebtoken');
const blacklist = new Set(); // Use Redis in production

const advancedTokenValidation = (token) => {
  // Check blacklist first
  if (blacklist.has(token)) {
    throw new Error('Token revoked');
  }

  const decoded = jwt.decode(token);
  if (!decoded) {
    throw new Error('Invalid token format');
  }

  // Validate required claims
  const requiredClaims = ['sub', 'iat', 'exp'];
  for (const claim of requiredClaims) {
    if (!decoded[claim]) {
      throw new Error(`Missing required claim: ${claim}`);
    }
  }

  // Validate issuer and audience
  if (decoded.iss !== 'your-app-name') {
    throw new Error('Invalid issuer');
  }

  if (decoded.aud !== 'your-app-audience') {
    throw new Error('Invalid audience');
  }

  return true;
};

const verifyToken = (token, secret) => {
  try {
    advancedTokenValidation(token);
    
    return jwt.verify(token, secret, {
      algorithms: ['HS256'], // Explicitly allow only secure algorithms
      ignoreExpiration: false,
      clockTolerance: 30, // 30 seconds tolerance for clock skew
    });
  } catch (error) {
    throw new Error(`Token verification failed: ${error.message}`);
  }
};