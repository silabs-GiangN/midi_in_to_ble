# Developer Services template project
** TODO **Put project introduction here. 1-2 sentence about the project.
## How to use the template
1. Import the [basic ruleset](.github/rulesets/Silabs-basic-ruleset-internal.json). Follow the official GitHub [guide](https://docs.github.com/en/enterprise-cloud@latest/organizations/managing-organization-settings/managing-rulesets-for-repositories-in-your-organization#importing-a-ruleset).
2. Create your sw projects under [projects/](projects/) folder.
3. Check [.gitignore](.gitignore) file and modify it if it is necessary
4. Check the [./Dockerfile](./Dockerfile) and extend it if necessary
5. Make sure that the whole project can be compiled with a single "make all" command.  
   also implement "make clean"
6. Fill out the [CODEOWNERS](./.github/CODEOWNERS) file. Here is the official github [guide](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-code-owners)
7. Update and extend if needed the [sonar project file](./.github/sonar-project.properties) .
8. **Internal projects only** Add a self-hosted runner with "devs-self-hosted-runner" name. Also add a "devs-self-hosted-runner" label to the runner.[guide](https://docs.github.com/en/actions/hosting-your-own-runners/managing-self-hosted-runners/adding-self-hosted-runners)
9. Secrets, add your SonarQube token and GitHub personal access token.
10. Check the workflow and adjust them according to the repo types (internal or public)
11. Update this Readme file and remove this list from it.


## Hardware requirements
** TODO ** List the required hw components.

## Hardware Setup
** TODO ** Create a block diagram about the components.

## Build environment setup
** TODO ** Add steps here how to create a build environment. Remove the not supported platforms.
### Docker
Using Docker for the build environment has several advantages:
- **Consistency**: Ensures the build environment is the same across all development machines.
- **Isolation**: Keeps the build environment isolated from the host machine, avoiding conflicts.
- **Portability**: Allows the build environment to be easily shared and reproduced.
- **Scalability**: Simplifies scaling the build process across multiple machines.

To set up the Docker-based build environment, follow these steps:
1. Install Docker on your machine. Only Linux and MacOS platforms are supported for now.
2. Clone the repository.
3. Build the Docker image using the provided [Dockerfile](./Dockerfile).
4. Run the Docker container with the necessary configurations.

### Windows
** TODO ** With a numbered list define the process how to set up a development environment.

### Linux
** TODO ** With a numbered list define the process how to set up a development environment.

### MacOS
** TODO ** With a numbered list define the process how to set up a development environment.

## Debug environment
**TODO**
Explain how can a developer debug this software project. Pictures are recommended.
## Contributing
Please follow the [CONTRIBUTING](./.github/CONTRIBUTING.md) guideline-

## License
See the [LICENSE.md](./LICENSE.md) file for details.

## Secrets
The following secrets are required for this project:
- `SONAR_TOKEN`: Token to access Sonarqube servers
