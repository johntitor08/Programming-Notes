// Enhanced logout
app.post('/logout', enhancedAuth, (req, res) => {
  const token = extractToken(req);
  
  // Add to blacklist
  blacklist.add(token);
  
  // Clear cookies
  res.clearCookie('accessToken');
  res.clearCookie('refreshToken');
  
  res.json({ message: 'Logged out successfully' });
});

// Logout all devices
app.post('/logout-all', enhancedAuth, (req, res) => {
  refreshTokenManager.revokeAllUserTokens(req.user.id);
  
  res.clearCookie('accessToken');
  res.clearCookie('refreshToken');
  
  res.json({ message: 'Logged out from all devices' });
});