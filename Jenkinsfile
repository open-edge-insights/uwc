pipeline {
    agent { label 'UWC' }
    options {
        timestamps()
    }
    environment {
        // optional, default 'checkmarx,protex', available: protex,checkmarx,sonarqube,klocwork,protecode
        // SCANNERS = 'checkmarx,protex'

        PROJECT_NAME = 'UWC'
        SLACK_SUCCESS = '#indu-uwc'
        SLACK_FAIL   = '#indu-uwc'
    }
    stages {
        stage('Protex Scan') {
            steps {
                    scanners = ['protex']
                    protexProjectName = 'UWC'
                    protexBuildName = 'rrp-generic-protex-build'
                }
		}
	}
}
