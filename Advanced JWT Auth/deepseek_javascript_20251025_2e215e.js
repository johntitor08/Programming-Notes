const securityConfig = {
  // Production settings
  production: {
    cookieSecure: true,
    sameSite: 'strict',
    hsts: true,
    contentSecurityPolicy: true
  },
  
  // Development settings
  development: {
    cookieSecure: false,
    sameSite: 'lax',
    hsts: false,
    contentSecurityPolicy: false
  }
};

const getSecurityConfig = () => {
  return securityConfig[process.env.NODE_ENV] || securityConfig.development;
};