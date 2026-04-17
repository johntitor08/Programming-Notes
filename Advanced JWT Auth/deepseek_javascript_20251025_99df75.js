const rateLimit = require('express-rate-limit');

// Rate limiting for auth endpoints
const authLimiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 5, // 5 attempts per window
  message: 'Too many authentication attempts, please try again later',
  standardHeaders: true,
  legacyHeaders: false,
});

// Enhanced token verification middleware
const enhancedAuth = (req, res, next) => {
  const token = extractToken(req);
  
  if (!token) {
    return res.status(401).json({ 
      error: 'ACCESS_DENIED',
      message: 'No token provided' 
    });
  }

  try {
    const decoded = verifyToken(token, JWT_CONFIG.accessSecret);
    
    // Add additional security checks
    if (isTokenCompromised(decoded)) {
      return res.status(401).json({
        error: 'TOKEN_COMPROMISED',
        message: 'Please re-authenticate'
      });
    }

    req.user = {
      id: decoded.sub,
      email: decoded.email,
      role: decoded.role,
      sessionId: decoded.sessionId // Track individual sessions
    };

    next();
  } catch (error) {
    // Don't reveal too much information in errors
    const safeError = error.message.includes('expired') 
      ? 'Token expired' 
      : 'Invalid token';

    res.status(401).json({
      error: 'INVALID_TOKEN',
      message: safeError
    });
  }
};

const extractToken = (req) => {
  // Check multiple sources
  if (req.headers.authorization?.startsWith('Bearer ')) {
    return req.headers.authorization.split(' ')[1];
  }
  
  if (req.cookies?.accessToken) {
    return req.cookies.accessToken;
  }
  
  return null;
};