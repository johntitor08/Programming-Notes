const crypto = require('crypto');

class RefreshTokenManager {
  constructor() {
    this.refreshTokens = new Map(); // Use database in production
  }

  generateRefreshToken(userId, userAgent, ip) {
    const tokenId = crypto.randomBytes(32).toString('hex');
    const token = crypto.randomBytes(64).toString('hex');
    
    const hashedToken = crypto.createHash('sha256').update(token).digest('hex');
    
    const refreshTokenData = {
      id: tokenId,
      userId,
      hashedToken,
      userAgent,
      ipAddress: ip,
      createdAt: new Date(),
      expiresAt: new Date(Date.now() + 7 * 24 * 60 * 60 * 1000), // 7 days
      isRevoked: false
    };

    this.refreshTokens.set(tokenId, refreshTokenData);
    
    return {
      token,
      id: tokenId,
      expiresAt: refreshTokenData.expiresAt
    };
  }

  async validateRefreshToken(token, tokenId, userAgent, ip) {
    const storedToken = this.refreshTokens.get(tokenId);
    
    if (!storedToken) {
      throw new Error('Invalid refresh token');
    }

    if (storedToken.isRevoked) {
      throw new Error('Refresh token revoked');
    }

    if (new Date() > storedToken.expiresAt) {
      this.refreshTokens.delete(tokenId);
      throw new Error('Refresh token expired');
    }

    // Verify token hash
    const hashedToken = crypto.createHash('sha256').update(token).digest('hex');
    if (hashedToken !== storedToken.hashedToken) {
      throw new Error('Invalid refresh token');
    }

    // Optional: Validate user agent and IP
    if (storedToken.userAgent !== userAgent) {
      console.warn('User agent mismatch for refresh token');
      // You might want to revoke the token here for security
    }

    return storedToken;
  }

  revokeRefreshToken(tokenId) {
    this.refreshTokens.delete(tokenId);
  }

  revokeAllUserTokens(userId) {
    for (const [tokenId, tokenData] of this.refreshTokens.entries()) {
      if (tokenData.userId === userId) {
        this.refreshTokens.delete(tokenId);
      }
    }
  }
}

const refreshTokenManager = new RefreshTokenManager();