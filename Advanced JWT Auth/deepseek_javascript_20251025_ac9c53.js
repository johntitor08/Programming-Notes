// Never hardcode secrets
const crypto = require('crypto');

// Generate strong secret
const generateSecret = () => {
  return crypto.randomBytes(64).toString('hex');
};

// Use environment variables with fallbacks
const JWT_CONFIG = {
  accessSecret: process.env.JWT_ACCESS_SECRET || generateSecret(),
  refreshSecret: process.env.JWT_REFRESH_SECRET || generateSecret(),
  accessExpiresIn: '15m',
  refreshExpiresIn: '7d'
};

// Validate secrets on startup
if (!process.env.JWT_ACCESS_SECRET) {
  console.warn('Warning: Using auto-generated JWT secret. Set JWT_ACCESS_SECRET in production!');
}