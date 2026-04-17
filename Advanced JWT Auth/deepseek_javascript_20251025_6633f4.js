const bcrypt = require('bcryptjs');
const rateLimit = require('express-rate-limit');

// Track failed login attempts
const failedAttempts = new Map();

const loginSecurity = {
  maxAttempts: 5,
  lockoutTime: 15 * 60 * 1000, // 15 minutes
  
  checkLockout: (ip, email) => {
    const key = `${ip}:${email}`;
    const attempts = failedAttempts.get(key);
    
    if (attempts && attempts.count >= loginSecurity.maxAttempts) {
      const timeSinceFirstAttempt = Date.now() - attempts.firstAttempt;
      if (timeSinceFirstAttempt < loginSecurity.lockoutTime) {
        throw new Error('Account temporarily locked due to too many failed attempts');
      } else {
        // Reset after lockout time
        failedAttempts.delete(key);
      }
    }
  },

  recordFailedAttempt: (ip, email) => {
    const key = `${ip}:${email}`;
    const attempts = failedAttempts.get(key) || { count: 0, firstAttempt: Date.now() };
    
    attempts.count++;
    failedAttempts.set(key, attempts);
  },

  clearFailedAttempts: (ip, email) => {
    const key = `${ip}:${email}`;
    failedAttempts.delete(key);
  }
};

// Enhanced login endpoint
app.post('/login', authLimiter, async (req, res) => {
  const { email, password } = req.body;
  const ip = req.ip;
  const userAgent = req.get('User-Agent');

  try {
    // Check for lockout
    loginSecurity.checkLockout(ip, email);

    // Input validation
    if (!email || !password) {
      return res.status(400).json({ 
        error: 'VALIDATION_ERROR',
        message: 'Email and password are required' 
      });
    }

    // Find user
    const user = await findUserByEmail(email);
    if (!user) {
      loginSecurity.recordFailedAttempt(ip, email);
      // Use generic error message to avoid user enumeration
      return res.status(401).json({ 
        error: 'AUTHENTICATION_FAILED',
        message: 'Invalid credentials' 
      });
    }

    // Verify password with timing-safe comparison
    const isValid = await bcrypt.compare(password, user.password);
    if (!isValid) {
      loginSecurity.recordFailedAttempt(ip, email);
      return res.status(401).json({ 
        error: 'AUTHENTICATION_FAILED',
        message: 'Invalid credentials' 
      });
    }

    // Clear failed attempts on successful login
    loginSecurity.clearFailedAttempts(ip, email);

    // Generate tokens with session ID
    const sessionId = crypto.randomBytes(16).toString('hex');
    
    const accessToken = jwt.sign(
      {
        sub: user.id,
        email: user.email,
        role: user.role,
        sessionId,
        iss: 'your-app-name',
        aud: 'your-app-audience'
      },
      JWT_CONFIG.accessSecret,
      { expiresIn: JWT_CONFIG.accessExpiresIn }
    );

    const refreshToken = refreshTokenManager.generateRefreshToken(
      user.id, 
      userAgent, 
      ip
    );

    // Set secure cookies
    res.cookie('accessToken', accessToken, {
      httpOnly: true,
      secure: process.env.NODE_ENV === 'production',
      sameSite: 'strict',
      maxAge: 15 * 60 * 1000 // 15 minutes
    });

    res.cookie('refreshToken', refreshToken.token, {
      httpOnly: true,
      secure: process.env.NODE_ENV === 'production',
      sameSite: 'strict',
      maxAge: 7 * 24 * 60 * 60 * 1000 // 7 days
    });

    res.json({
      success: true,
      user: {
        id: user.id,
        email: user.email,
        role: user.role
      },
      sessionId
    });

  } catch (error) {
    res.status(500).json({
      error: 'LOGIN_ERROR',
      message: 'Authentication failed'
    });
  }
});